#include "ParticleManager.h"
#include "ModelBinaryLoader.h"
#include "WrapperFactory.h"
#include "Utilities.h"
#include <random>


ParticleManager::ParticleManager(Graphics *p_Graphics) 
	: m_Graphics(p_Graphics),
	m_BallModel(nullptr),
	m_Constant(nullptr),
	m_ParticleRenderData(nullptr),
	m_InstanceRender(nullptr)
{
}


ParticleManager::~ParticleManager(void)
{
	SAFE_DELETE(m_BallModel);
	SAFE_DELETE(m_Constant);
	SAFE_DELETE(m_InstanceRender);
	SAFE_DELETE(m_ParticleRenderData);
}

void ParticleManager::init()
{
	loadBallModel();
	createShaders();

	Particle p;
	std::default_random_engine generator;
	std::uniform_real_distribution<float> color(0.f,1.f);
	std::uniform_real_distribution<float> position(-30.f,30.f);

	p.m_Scale = 2;

	for(int i = 0; i < 50; i++)
	{
		p.m_Position = DirectX::XMFLOAT3(position(generator),position(generator),position(generator));
		p.m_Color = DirectX::XMFLOAT3(color(generator),color(generator),color(generator));

		m_Particles.push_back(p);
	}


	Buffer::Description cbdesc;
	cbdesc.sizeOfElement = sizeof(Particle);
	cbdesc.type = Buffer::Type::VERTEX_BUFFER;
	cbdesc.usage = Buffer::Usage::USAGE_IMMUTABLE;
	cbdesc.initData = m_Particles.data();
	cbdesc.numOfElements = m_Particles.size();
	m_ParticleRenderData = WrapperFactory::getInstance()->createBuffer(cbdesc);


	cbdesc.sizeOfElement = sizeof(constantBuffer);
	cbdesc.initData = nullptr;
	cbdesc.numOfElements = 1;
	cbdesc.usage = Buffer::Usage::DEFAULT;
	cbdesc.type = Buffer::Type::CONSTANT_BUFFER_ALL;
	m_Constant = WrapperFactory::getInstance()->createBuffer(cbdesc);
	int dummy = 0;
}

void ParticleManager::update(float p_Dt)
{

}

void ParticleManager::render()
{
	m_Graphics->activateRT();
	m_Graphics->getDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_Constant->setBuffer(0);

	UINT Offsets[2] = {0,0};
	ID3D11Buffer * buffers[] = {m_BallModel->getBufferPointer(), m_ParticleRenderData->getBufferPointer()};
	UINT Stride[2] = {sizeof(DirectX::XMFLOAT3), sizeof(Particle)};

	m_InstanceRender->setShader();
	m_Graphics->getDeviceContext()->IASetVertexBuffers(0,2,buffers,Stride, Offsets);

	m_Graphics->getDeviceContext()->DrawInstanced(m_BallModel->getNumOfElements(), m_Particles.size(),0,0);

	ID3D11Buffer* nullBuffers[] = { nullptr, nullptr };
	m_Graphics->getDeviceContext()->IASetVertexBuffers(0, 2, nullBuffers, Stride, Offsets);
	m_InstanceRender->unSetShader();
	m_Constant->unsetBuffer(0);
}

void ParticleManager::updateCameraInformation(DirectX::XMFLOAT4X4 &p_View, DirectX::XMFLOAT4X4 &p_Projection)
{
	constantBuffer cb;
	cb.m_View = p_View;
	cb.m_Projection = p_Projection;

	m_Graphics->getDeviceContext()->UpdateSubresource(m_Constant->getBufferPointer(), NULL,NULL, &cb,NULL,NULL);
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

	m_InstanceRender = WrapperFactory::getInstance()->createShader(L"./InstanceRender.hlsl", nullptr,
		"PointLightVS,PointLightPS", "5_0",ShaderType::VERTEX_SHADER | ShaderType::PIXEL_SHADER, shaderDesc, 4);
}
