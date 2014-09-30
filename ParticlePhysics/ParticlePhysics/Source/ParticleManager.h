#pragma once
#include "Graphics.h"
#include "Buffer.h"
#include "Shader.h"
#include "ComputeHelp.h"
#include "GPUTimer.h"

#include <vector>


//typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT3 float3;
//typedef DirectX::XMFLOAT2 float2;
//typedef DirectX::XMFLOAT4X4 float4x4;

struct ParticlePhysics
{
	float3 m_Position;
	float3 m_Velocity;
	float3 m_Acceleration;
	float3 m_Forces;
	float3 m_Impulses;
	float m_Mass;
};


struct Particle
{
	float3 m_Position;
	float3 m_Color;
	float m_Scale;
};


class ParticleManager
{
private:
	struct constantBuffer
	{
		DirectX::XMFLOAT4X4 m_View;
		DirectX::XMFLOAT4X4 m_Projection;
	};

	Graphics *m_Graphics;
	Buffer *m_BallModel;
	Buffer *m_Constant;
	Buffer *m_ConstantDeltaTime;
	Buffer *m_ParticleRenderData;
	Shader *m_InstanceRender;
	ID3D11DepthStencilState	*m_DepthStencilState;
	ID3D11RasterizerState *m_RasterState;

	ComputeWrap *m_ComputeSys;
	ComputeShader *m_ComputeShader;
	ComputeBuffer *m_ComputeBuffer;

	GPUTimer *m_Timer;

	std::vector<Particle> m_Particles;
	std::vector<ParticlePhysics> m_ParticlesPhysics;

public:
	ParticleManager(Graphics *p_Graphics);
	~ParticleManager(void);

	void init();

	void update(float p_Dt);

	void render();

	void updateCameraInformation(DirectX::XMFLOAT4X4 &p_View, DirectX::XMFLOAT4X4 &p_Projection);

	float getGPUTime();

private:
	void loadBallModel();
	void createShaders();

	void createRenderStates();
};

