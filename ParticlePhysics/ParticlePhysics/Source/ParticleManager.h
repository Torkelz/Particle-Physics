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
	float3 m_Color;
	float m_Radius;
};


struct Particle
{
	float3 m_Position;
	float3 m_Color;
	float m_Radius;
};


class ParticleManager
{
private:
	struct constantBuffer
	{
		DirectX::XMFLOAT4X4 m_View;
		DirectX::XMFLOAT4X4 m_Projection;
		DirectX::XMFLOAT4X4 m_InverseProjection;
	};
	struct constantBufferFrame
	{
		float m_DT;
		int m_NrThreads;
	};

	Graphics *m_Graphics;
	Buffer *m_Constant;
	Buffer *m_ConstantDeltaTime;
	//Shader *m_InstanceRender;

	ComputeWrap *m_ComputeSys;
	ComputeShader *m_ComputeShader;
	ComputeBuffer *m_ComputeBuffer;
	ComputeBuffer *m_ComputeAppendBuffer;

	GPUTimer *m_Timer;

	std::vector<ParticlePhysics> m_ParticlesPhysics;

	unsigned int numActiveElements;

	ID3D11Texture2D *m_DepthStencilBuffer;
	ID3D11ShaderResourceView *m_DepthView;
	
	Shader* m_Particle;
	ID3D11ShaderResourceView* m_Texture;
	ID3D11SamplerState* m_SamplerState;
	ID3D11Buffer* m_IndirectBuffer;
	ID3D11Buffer* m_StagingBuffer;

public:
	ParticleManager(Graphics *p_Graphics);
	~ParticleManager(void);

	void init();

	void update(float p_Dt);

	void render();

	void updateCameraInformation(DirectX::XMFLOAT4X4 &p_View, DirectX::XMFLOAT4X4 &p_Projection);

	float getGPUTime();
	unsigned int getNumParticles();

private:
	void loadBallModel();
	void createShaders();

	void createRenderStates();

	ID3D11RenderTargetView *createRenderTarget(D3D11_TEXTURE2D_DESC &desc);
	ID3D11ShaderResourceView *createShaderResourceView(D3D11_TEXTURE2D_DESC &desc, ID3D11RenderTargetView *p_Rendertarget);
};

