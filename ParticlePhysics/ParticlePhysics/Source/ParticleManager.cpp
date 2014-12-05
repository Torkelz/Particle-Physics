#include "ParticleManager.h"
#include "ModelBinaryLoader.h"
#include "WrapperFactory.h"
#include "Utilities.h"
#include "Window.h"
#include "UserExceptions.h"

#include "WICTextureLoader.h"

#include <random>


ParticleManager::ParticleManager(Graphics *p_Graphics) 
	: m_Graphics(p_Graphics),
	m_Constant(nullptr),
	m_ConstantDeltaTime(nullptr),
	//m_InstanceRender(nullptr),
	m_ComputeSys(nullptr),
	m_ComputeShader(nullptr),
	m_ComputeBuffer(nullptr),
	m_ComputeAppendBuffer(nullptr),
	m_Timer(nullptr),
	m_DepthStencilBuffer(nullptr),
	m_DepthView(nullptr),
	m_Particle(nullptr),
	m_Texture(nullptr),
	m_SamplerState(nullptr),
	m_IndirectBuffer(nullptr)
{
}


ParticleManager::~ParticleManager(void)
{
	SAFE_DELETE(m_Constant);
	SAFE_DELETE(m_ConstantDeltaTime);
	//SAFE_DELETE(m_InstanceRender);
	SAFE_DELETE(m_ComputeShader);
	SAFE_DELETE(m_ComputeSys);
	SAFE_DELETE(m_ComputeBuffer);
	SAFE_DELETE(m_ComputeAppendBuffer);
	SAFE_DELETE(m_Timer);

	SAFE_RELEASE(m_DepthStencilBuffer);
	SAFE_RELEASE(m_DepthView);

	SAFE_DELETE(m_Particle);
	SAFE_RELEASE(m_Texture);
	SAFE_RELEASE(m_SamplerState);
	SAFE_RELEASE(m_IndirectBuffer);
}

void ParticleManager::init()
{
	createShaders();
	createRenderStates();
	m_Timer = new GPUTimer(m_Graphics->getDevice(), m_Graphics->getDeviceContext());

	Particle p;
	ParticlePhysics physics;
	ZeroMemory(&physics, sizeof(ParticlePhysics));

	std::default_random_engine generator;
	std::uniform_real_distribution<float> color(0.f,1.f);
	std::uniform_real_distribution<float> position(-30.f,30.f);

	p.m_Radius = 1;
	numActiveElements = 500000;
	for (unsigned int i = 0; i < numActiveElements; i++)
	{
		p.m_Position = DirectX::XMFLOAT3(position(generator),position(generator),position(generator));
		p.m_Color = DirectX::XMFLOAT3(color(generator),color(generator),color(generator));

		m_Particles.push_back(p);

		physics.m_Position = p.m_Position;
		physics.m_Mass = 1.f;
		using DirectX::operator*;
		DirectX::XMStoreFloat3(&physics.m_Forces, DirectX::XMVector3Normalize(-1.0f * DirectX::XMLoadFloat3(&p.m_Position)));

		m_ParticlesPhysics.push_back(physics);
	}

	Buffer::Description cbdesc = {};
	cbdesc.sizeOfElement = sizeof(constantBuffer);
	cbdesc.initData = nullptr;
	cbdesc.numOfElements = 1;
	cbdesc.usage = Buffer::Usage::DEFAULT;
	cbdesc.type = Buffer::Type::CONSTANT_BUFFER_ALL;
	m_Constant = WrapperFactory::getInstance()->createBuffer(cbdesc);

	cbdesc.sizeOfElement = sizeof(float);
	cbdesc.initData = nullptr;
	cbdesc.numOfElements = 1;
	cbdesc.usage = Buffer::Usage::DEFAULT;
	cbdesc.type = Buffer::Type::CONSTANT_BUFFER_ALL;
	m_ConstantDeltaTime = WrapperFactory::getInstance()->createBuffer(cbdesc);

	m_ComputeBuffer = m_ComputeSys->CreateBuffer(STRUCTURED_BUFFER, sizeof(ParticlePhysics), m_ParticlesPhysics.size(), false, true, false, m_ParticlesPhysics.data(), true);
	
	m_ComputeAppendBuffer = m_ComputeSys->CreateBuffer(STRUCTURED_BUFFER, sizeof(Particle), m_Particles.size(), true, true, true, nullptr, true);

	DirectX::CreateWICTextureFromFile(m_Graphics->getDevice(), m_Graphics->getDeviceContext(), L".\\Depthmap.png", nullptr, &m_Texture);

	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sd.MinLOD = 0;
	sd.MaxLOD = D3D11_FLOAT32_MAX;
	m_Graphics->getDevice()->CreateSamplerState(&sd, &m_SamplerState);
	
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.ByteWidth = sizeof(UINT) * 4;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	bufferDesc.StructureByteStride = 0;//sizeof(UINT);
	D3D11_SUBRESOURCE_DATA initData;
	UINT bufferInit[4] = { numActiveElements, 1, 0, 0 };
	initData.pSysMem = bufferInit;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	m_Graphics->getDevice()->CreateBuffer(&bufferDesc, &initData, &m_IndirectBuffer);
}

void ParticleManager::update(float p_Dt)
{
	m_Graphics->getDeviceContext()->UpdateSubresource(m_ConstantDeltaTime->getBufferPointer(), NULL,NULL, &p_Dt,NULL,NULL);

	static ID3D11UnorderedAccessView* cbUAV[] = {m_ComputeBuffer->GetUnorderedAccessView()};
	static ID3D11UnorderedAccessView* cabUAV[] = { m_ComputeAppendBuffer->GetUnorderedAccessView() };
	static ID3D11UnorderedAccessView* nullUAVs[] = { 0, 0 };
	static UINT initialUAV[] = { 0 };
	static ID3D11ShaderResourceView* srv[] = { m_DepthView };
	static ID3D11ShaderResourceView* nullsrv[] = { 0 };

	//m_Timer->Start();
	m_ComputeShader->Set();
	m_Graphics->getDeviceContext()->CSSetUnorderedAccessViews(0, 1, cbUAV, NULL);
	m_Graphics->getDeviceContext()->CSSetUnorderedAccessViews(1, 1, cabUAV, initialUAV);
	m_Graphics->getDeviceContext()->CSSetShaderResources(0, 1, srv);
	m_Graphics->getDeviceContext()->CSSetSamplers(0, 1, &m_SamplerState);
	m_ConstantDeltaTime->setBuffer(0);
	m_Constant->setBuffer(1);

	m_Graphics->getDeviceContext()->Dispatch(4/*32*/,1,1);

	m_ComputeShader->Unset();
	m_Constant->unsetBuffer(1);
	m_Graphics->getDeviceContext()->CSSetShaderResources(0, 1, nullsrv);

	m_Graphics->getDeviceContext()->CSSetUnorderedAccessViews(0, 2, nullUAVs, NULL);
	//m_Timer->Stop();
}

void ParticleManager::render()
{
	m_Graphics->setRT();

	static ID3D11ShaderResourceView *srv[] = { m_ComputeAppendBuffer->GetResourceView() };
	static ID3D11ShaderResourceView *nullsrv[] = { 0 };

	m_Graphics->getDeviceContext()->VSSetShaderResources(0, 1, srv);
	m_Graphics->getDeviceContext()->PSSetShaderResources(0, 1, &m_Texture);
	m_Graphics->getDeviceContext()->PSSetSamplers(0, 1, &m_SamplerState);
	m_Graphics->getDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	m_Constant->setBuffer(0);
	m_Particle->setShader();

	m_Graphics->getDeviceContext()->DrawInstancedIndirect(m_IndirectBuffer, 0);

	m_Particle->unSetShader();
	m_Constant->unsetBuffer(0);
	m_Graphics->unsetRT();
	m_Graphics->getDeviceContext()->VSSetShaderResources(0, 1, nullsrv);
}

void ParticleManager::updateCameraInformation(DirectX::XMFLOAT4X4 &p_View, DirectX::XMFLOAT4X4 &p_Projection)
{
	constantBuffer cb;
	cb.m_View = p_View;
	cb.m_Projection = p_Projection;
	XMStoreFloat4x4(&cb.m_InverseProjection, XMMatrixInverse(nullptr, XMLoadFloat4x4(&p_Projection)));

	m_Graphics->getDeviceContext()->UpdateSubresource(m_Constant->getBufferPointer(), NULL,NULL, &cb,NULL,NULL);
}

float ParticleManager::getGPUTime()
{
	if(m_Timer)
		return (float)m_Timer->GetTime();
	else
		return -1.f;
}

void ParticleManager::createShaders()
{
	//ShaderInputElementDescription shaderDesc[] = 
	//{
	//	{"POSITION",	0, Format::R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA,	  0},
	//	{"LPOSITION",	0, Format::R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA,  1},
	//	{"COLOR",		0, Format::R32G32B32_FLOAT, 1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1},
	//	{"RANGE",		0, Format::R32_FLOAT,		1, 24, D3D11_INPUT_PER_INSTANCE_DATA, 1},
	//};

	//m_InstanceRender = WrapperFactory::getInstance()->createShader(L"Bin/assets/shaders/InstanceRender.hlsl", nullptr,
	//	"PointLightVS,PointLightPS", "5_0",ShaderType::VERTEX_SHADER | ShaderType::PIXEL_SHADER, shaderDesc, 4);


	m_ComputeSys = new ComputeWrap(m_Graphics->getDevice(), m_Graphics->getDeviceContext());

	m_ComputeShader = m_ComputeSys->CreateComputeShader(_T("Bin/assets/shaders/ComputeShader.hlsl"), nullptr, "main", nullptr);

	m_Particle = WrapperFactory::getInstance()->createShader(L"PixelShader.hlsl", "VS,PS,GS", "5_0", ShaderType::VERTEX_SHADER | ShaderType::PIXEL_SHADER | ShaderType::GEOMETRY_SHADER);
}

void ParticleManager::createRenderStates()
{
	D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_R32_FLOAT;// depthBufferDesc.Format;
	desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = 1;
	desc.Texture2D.MostDetailedMip = 0;

	m_Graphics->getDevice()->CreateShaderResourceView(m_Graphics->getDepthTexture(), &desc, &m_DepthView);
}

unsigned int ParticleManager::getNumParticles()
{
	return numActiveElements;
}

ID3D11RenderTargetView *ParticleManager::createRenderTarget(D3D11_TEXTURE2D_DESC &desc)
{
	// Create the render target texture
	HRESULT result = S_FALSE;
	ID3D11RenderTargetView* ret = nullptr;
	//Create the render targets
	D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
	rtDesc.Format = desc.Format;
	rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtDesc.Texture2D.MipSlice = 0;
	ID3D11Texture2D *temp;
	result = m_Graphics->getDevice()->CreateTexture2D(&desc, nullptr, &temp);
	if (FAILED(result))
		throw GraphicsException("Error creating Texture2D while creating a render target.", __LINE__, __FILE__);
	result = m_Graphics->getDevice()->CreateRenderTargetView(temp, &rtDesc, &ret);
	SAFE_RELEASE(temp);
	if (FAILED(result))
		throw GraphicsException("Error creating a render target.", __LINE__, __FILE__);

	return ret;
}
ID3D11ShaderResourceView *ParticleManager::createShaderResourceView(D3D11_TEXTURE2D_DESC &desc, ID3D11RenderTargetView *p_Rendertarget)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC dssrvdesc;
	dssrvdesc.Format = desc.Format;
	dssrvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	dssrvdesc.Texture2D.MipLevels = 1;
	dssrvdesc.Texture2D.MostDetailedMip = 0;
	ID3D11ShaderResourceView* ret = nullptr;
	ID3D11Resource* tt;
	p_Rendertarget->GetResource(&tt);
	HRESULT result = m_Graphics->getDevice()->CreateShaderResourceView(tt, &dssrvdesc, &ret);

	SAFE_RELEASE(tt);
	if (FAILED(result))
		throw GraphicsException("Error when creating shader resource view from render target.", __LINE__, __FILE__);
	return ret;
}