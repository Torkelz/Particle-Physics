#ifndef COMPUTE
#define COMPUTE
#include "Structs.hlsli"

RWStructuredBuffer<ParticlePhysics> m_Particles : register(u0);

cbuffer cBufferdata : register(b0)
{
	float m_DeltaTime;
};

[numthreads(128, 1, 1)]
void main( uint3 ThreadID : SV_DispatchThreadID )
{
	uint nrThreads = 128*4;

	uint dimension;
	uint stride;
	m_Particles.GetDimensions(dimension, stride);


	for(int pIndex = ThreadID.x; pIndex < dimension; pIndex += nrThreads)
	{
		ParticlePhysics p = m_Particles[pIndex];

		if(p.m_Mass == 0.0f)
			return;

		float3 F = float3(0.f,0.f,0.f);
		F += p.m_Forces;
		F += p.m_Impulses;
		p.m_Impulses = float3(0.f,0.f,0.f);

		p.m_Acceleration = F / p.m_Mass;
		p.m_Velocity += p.m_Acceleration * m_DeltaTime;
		p.m_Position += p.m_Velocity * m_DeltaTime;

		m_Particles[pIndex] = p;
	}
}
#endif