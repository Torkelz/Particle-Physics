#include "ParticleManager.h"
#include "ModelBinaryLoader.h"
#include "WrapperFactory.h"
#include "Utilities.h"
#include "Window.h"
#include "UserExceptions.h"
#include <random>


ParticleManager::ParticleManager(Graphics *p_Graphics) 
	: m_Graphics(p_Graphics),
	m_BallModel(nullptr),
	m_Constant(nullptr),
	m_ConstantDeltaTime(nullptr),
	m_ParticleRenderData(nullptr),
	m_InstanceRender(nullptr),
	m_DepthStencilState(nullptr),
	m_RasterState(nullptr),
	m_ComputeSys(nullptr),
	m_ComputeShader(nullptr),
	m_ComputeBuffer(nullptr),
	m_ComputeAppendBuffer(nullptr),
	m_Timer(nullptr),
	m_DepthStencilBuffer(nullptr),
	m_DepthView(nullptr)
{
}


ParticleManager::~ParticleManager(void)
{
	SAFE_DELETE(m_BallModel);
	SAFE_DELETE(m_Constant);
	SAFE_DELETE(m_ConstantDeltaTime);
	SAFE_DELETE(m_InstanceRender);
	SAFE_DELETE(m_ParticleRenderData);
	SAFE_RELEASE(m_DepthStencilState);
	SAFE_RELEASE(m_RasterState);
	SAFE_DELETE(m_ComputeShader);
	SAFE_DELETE(m_ComputeSys);
	SAFE_DELETE(m_ComputeBuffer);
	SAFE_DELETE(m_ComputeAppendBuffer);
	SAFE_DELETE(m_Timer);

	SAFE_RELEASE(m_DepthStencilBuffer);
	SAFE_RELEASE(m_DepthView);
}

void ParticleManager::init()
{
	loadBallModel();
	createShaders();
	createRenderStates();
	m_Timer = new GPUTimer(m_Graphics->getDevice(), m_Graphics->getDeviceContext());

	Particle p;
	ParticlePhysics physics;
	ZeroMemory(&physics, sizeof(ParticlePhysics));

	std::default_random_engine generator;
	std::uniform_real_distribution<float> color(0.f,1.f);
	std::uniform_real_distribution<float> position(-30.f,30.f);

	p.m_Scale = 2;
	numActiveElements = 50000;
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


	Buffer::Description cbdesc;
	cbdesc.sizeOfElement = sizeof(Particle);
	cbdesc.type = Buffer::Type::VERTEX_BUFFER;
	cbdesc.usage = Buffer::Usage::CPU_WRITE;
	cbdesc.initData = m_Particles.data();
	cbdesc.numOfElements = m_Particles.size();
	m_ParticleRenderData = WrapperFactory::getInstance()->createBuffer(cbdesc);


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
	
	m_ComputeAppendBuffer = m_ComputeSys->CreateBuffer(STRUCTURED_BUFFER, sizeof(Particle), m_Particles.size(), false, true, true, nullptr, true);



	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = 1280;
	desc.Height = 720;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	m_Depth = createRenderTarget(desc);
	m_DepthSRV = createShaderResourceView(desc, m_Depth);
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

	m_Timer->Start();
	m_ComputeShader->Set();
	m_Graphics->getDeviceContext()->CSSetUnorderedAccessViews(0, 1, cbUAV, NULL);
	m_Graphics->getDeviceContext()->CSSetUnorderedAccessViews(1, 1, cabUAV, initialUAV);
	m_Graphics->getDeviceContext()->CSSetShaderResources(0, 1, srv);
	m_ConstantDeltaTime->setBuffer(0);
	m_Constant->setBuffer(1);

	m_Graphics->getDeviceContext()->Dispatch(4/*32*/,1,1);

	m_ComputeShader->Unset();
	m_Constant->unsetBuffer(1);
	m_Graphics->getDeviceContext()->CSSetShaderResources(0, 1, nullsrv);

	//m_Graphics->getDeviceContext()->CSSetUnorderedAccessViews(0, 2, nullUAVs, NULL);

	//m_ComputeBuffer->CopyToStaging();

	//D3D11_MAPPED_SUBRESOURCE particle;
	//m_Graphics->getDeviceContext()->Map(m_ParticleRenderData->getBufferPointer(), NULL, D3D11_MAP_WRITE_NO_OVERWRITE, NULL, &particle);

	//Particle *ptr = (Particle *)particle.pData;
	//ParticlePhysics *pPhysics = m_ComputeBuffer->Map<ParticlePhysics>();
	//
	//for(unsigned int j = 0; j < m_Particles.size(); j++)
	//{
	//	ptr->m_Position = pPhysics->m_Position;
	//	ptr++;
	//	pPhysics++;
	//}

	//m_Graphics->getDeviceContext()->Unmap(m_ParticleRenderData->getBufferPointer(), NULL);
	//m_ComputeBuffer->Unmap();
	
	//m_Graphics->getDeviceContext()->CopyStructureCount(m_ComputeAppendBuffer->GetStaging(), 0, m_ComputeAppendBuffer->GetUnorderedAccessView());
	//m_ComputeAppendBuffer->CopyToStaging();
	m_Graphics->getDeviceContext()->CopyResource(m_ParticleRenderData->getBufferPointer(), m_ComputeAppendBuffer->GetResource());

	m_Graphics->getDeviceContext()->CopyStructureCount(m_ComputeAppendBuffer->GetStaging(), 0, m_ComputeAppendBuffer->GetUnorderedAccessView());
	D3D11_MAPPED_SUBRESOURCE subresource;
	m_Graphics->getDeviceContext()->Map(m_ComputeAppendBuffer->GetStaging(), 0, D3D11_MAP_READ, 0, &subresource);
	numActiveElements = *(unsigned int*)subresource.pData;
	m_Graphics->getDeviceContext()->Unmap(m_ComputeAppendBuffer->GetStaging(), 0);

	if (numActiveElements < 50000)
		int dummy = 0;


	m_Timer->Stop();
}

void ParticleManager::render()
{
	//m_Graphics->setRT();
	ID3D11RenderTargetView *rtv[] = { m_Graphics->getRTV(), m_Depth };
	ID3D11RenderTargetView *nullrtv[] = { 0,0 };

	m_Graphics->getDeviceContext()->OMSetRenderTargets(2, rtv, m_Graphics->getDSV());
	m_Graphics->getDeviceContext()->RSSetState(m_RasterState);
	m_Graphics->getDeviceContext()->OMSetDepthStencilState(m_DepthStencilState,0);
	m_Graphics->getDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_Constant->setBuffer(0);

	UINT Offsets[2] = {0,0};
	ID3D11Buffer * buffers[] = {m_BallModel->getBufferPointer(), m_ParticleRenderData->getBufferPointer()};
	UINT Stride[2] = {sizeof(DirectX::XMFLOAT3), sizeof(Particle)};

	m_InstanceRender->setShader();
	m_Graphics->getDeviceContext()->IASetVertexBuffers(0,2,buffers,Stride, Offsets);

	//static const unsigned int countPerInstance = 1000;
	//unsigned int nrCalls = m_Particles.size() / countPerInstance;
	//for (unsigned int i = 0; i < nrCalls; ++i)
	if (numActiveElements > 0)
		m_Graphics->getDeviceContext()->DrawInstanced(m_BallModel->getNumOfElements(), numActiveElements, 0, 0);

	ID3D11Buffer* nullBuffers[] = { nullptr, nullptr };
	m_Graphics->getDeviceContext()->IASetVertexBuffers(0, 2, nullBuffers, Stride, Offsets);
	m_InstanceRender->unSetShader();
	m_Constant->unsetBuffer(0);

	m_Graphics->getDeviceContext()->OMSetRenderTargets(2, nullrtv, NULL);
	//m_Graphics->unsetRT();
	m_Graphics->getDeviceContext()->CopyResource(m_DepthStencilBuffer, m_Graphics->getDepthTexture());
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

void ParticleManager::loadBallModel()
{
	Buffer::Description cbdesc;
	cbdesc.sizeOfElement = sizeof(DirectX::XMFLOAT3);
	cbdesc.type = Buffer::Type::VERTEX_BUFFER;
	cbdesc.usage = Buffer::Usage::USAGE_IMMUTABLE;

	ModelBinaryLoader modelLoader;
	modelLoader.loadBinaryFile("assets/models/Sphere2.btx");
	const std::vector<StaticVertex>& vertices = modelLoader.getStaticVertexBuffer();
	std::vector<DirectX::XMFLOAT3> temp;
	for(unsigned int i = 0; i < vertices.size(); i++)
	{
		temp.push_back(DirectX::XMFLOAT3(vertices.at(i).m_Position.x,vertices.at(i).m_Position.y,vertices.at(i).m_Position.z));
	}

	cbdesc.initData = temp.data();
	cbdesc.numOfElements = temp.size();
	m_BallModel = WrapperFactory::getInstance()->createBuffer(cbdesc);
}

void ParticleManager::createShaders()
{
	ShaderInputElementDescription shaderDesc[] = 
	{
		{"POSITION",	0, Format::R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA,	  0},
		{"LPOSITION",	0, Format::R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA,  1},
		{"COLOR",		0, Format::R32G32B32_FLOAT, 1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{"RANGE",		0, Format::R32_FLOAT,		1, 24, D3D11_INPUT_PER_INSTANCE_DATA, 1},
	};

	m_InstanceRender = WrapperFactory::getInstance()->createShader(L"Bin/assets/shaders/InstanceRender.hlsl", nullptr,
		"PointLightVS,PointLightPS", "5_0",ShaderType::VERTEX_SHADER | ShaderType::PIXEL_SHADER, shaderDesc, 4);


	m_ComputeSys = new ComputeWrap(m_Graphics->getDevice(), m_Graphics->getDeviceContext());

	m_ComputeShader = m_ComputeSys->CreateComputeShader(_T("Bin/assets/shaders/ComputeShader.hlsl"), nullptr, "main", nullptr);
}

void ParticleManager::createRenderStates()
{
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

	//Initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	//Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	//Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	m_Graphics->getDevice()->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilState);

	D3D11_RASTERIZER_DESC rasterDesc;

	//Setup the raster description which will determine how and what polygons will be drawn.
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	m_Graphics->getDevice()->CreateRasterizerState(&rasterDesc, &m_RasterState);






	D3D11_TEXTURE2D_DESC depthBufferDesc;

	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));


	//Set up the description of the depth buffer.
	depthBufferDesc.Width = 1280;
	depthBufferDesc.Height = 720;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS;// DXGI_FORMAT_R24G8_TYPELESS;//DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	//Create the texture for the depth buffer using the filled out description.
	m_Graphics->getDevice()->CreateTexture2D(&depthBufferDesc, NULL, &m_DepthStencilBuffer);
	D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};

	desc.Format = DXGI_FORMAT_R32_FLOAT;// depthBufferDesc.Format;
	desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = 1;
	desc.Texture2D.MostDetailedMip = 0;


	HRESULT r = m_Graphics->getDevice()->CreateShaderResourceView(m_DepthStencilBuffer, &desc, &m_DepthView);

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