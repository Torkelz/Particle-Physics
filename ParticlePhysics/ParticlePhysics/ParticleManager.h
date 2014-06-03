#pragma once
#include "Graphics.h"
#include "Buffer.h"
#include "Shader.h"
#include <vector>

//typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT3 float3;
//typedef DirectX::XMFLOAT2 float2;
//typedef DirectX::XMFLOAT4X4 float4x4;

#include "Structs.hlsli"


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
	Buffer *m_ParticleRenderData;
	Shader *m_InstanceRender;

	std::vector<Particle> m_Particles;
public:
	ParticleManager(Graphics *p_Graphics);
	~ParticleManager(void);

	void init();

	void update(float p_Dt);

	void render();

	void updateCameraInformation(DirectX::XMFLOAT4X4 &p_View, DirectX::XMFLOAT4X4 &p_Projection);

private:
	void loadBallModel();
	void createShaders();
};

