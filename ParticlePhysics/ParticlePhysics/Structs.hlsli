#ifndef STRUCTS
#define STRUCTS


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
#endif // STRUCTS