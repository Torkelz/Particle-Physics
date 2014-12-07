#ifndef COMPUTE
#define COMPUTE

#pragma pack_matrix(row_major)

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

RWStructuredBuffer<ParticlePhysics> m_Particles : register(u0);
AppendStructuredBuffer<Particle> m_output : register(u1);
Texture2D m_Depth : register(t0);
SamplerState m_textureSampler : register(s0);

cbuffer cBufferdata : register(b0)
{
	float m_DeltaTime;
	int m_NrThreads;
};

cbuffer cb : register(b1)
{
	float4x4	view;
	float4x4	projection;
	float4x4	inverseprojection;
};


bool isFullyOccluded(float2 pos1, float2 pos2, float r1, float r2)
{
	float dist = length(pos1 - pos2);

	if ((dist + r1) < r2)
		return true;
	else
		return false;
}

[numthreads(512, 1, 1)]
void main( uint3 ThreadID : SV_DispatchThreadID )
{
	uint nrThreads = 512 * m_NrThreads;

	uint dimension;
	uint stride;
	m_Particles.GetDimensions(dimension, stride);

	[allow_uav_condition]
	for (unsigned int pIndex = ThreadID.x; pIndex < dimension; pIndex += nrThreads)
	{
		ParticlePhysics p = m_Particles[pIndex];

		if (p.m_Mass == 0.0f)
			return;

		float3 F = float3(0.f, 0.f, 0.f);
			F += p.m_Forces;
		F += p.m_Impulses;
		p.m_Impulses = float3(0.f, 0.f, 0.f);

		float3 prevPosition = p.m_Position;

		p.m_Acceleration = F / p.m_Mass;
		p.m_Velocity += p.m_Acceleration * m_DeltaTime;
		p.m_Position += p.m_Velocity * m_DeltaTime;

		m_Particles[pIndex] = p;




		//Culling http://stackoverflow.com/questions/3717226/radius-of-projected-sphere

		//float4 preViewPos = mul(view, float4(prevPosition, 1.f));

		//float4 viewPos = mul(view, float4(p.m_Position, 1.f));
		//float4 projPos = mul(projection, viewPos);

		//float4 ndc = projPos / projPos.w;

		//uint3 texCoord = uint3((ndc.x + 1.f) * 0.5f, (ndc.y + 1.f) * 0.5f, 0);
		//float d = m_Depth.Load(texCoord).x;

		////float z_e = 2.0 * 1 * 1000 / (1000 + 1 - d * (1000 - 1));

		////float viewD = projection._43 / (d - projection._33);

		//// Get the depth value for this pixel
		//// Get x/w and y/w from the viewport position
		//float x = texCoord.x * 2 - 1;
		//float y = (1 - texCoord.y) * 2 - 1;
		//float4 vProjectedPos = float4(x, y, d, 1.0f);
		//// Transform by the inverse projection matrix
		//float4 vPositionVS = mul(inverseprojection, vProjectedPos);
		//// Divide by w to get the view-space position
		//float3 VV = vPositionVS.xyz / vPositionVS.w;

		//if (VV.z < viewPos.z)
		//	return;

		/*if (preViewPos.z + 2 < z_e)
			return;*/

		//for (unsigned int i = 0; i < 1000; ++i)
		//{
		//	if (i == pIndex)
		//		continue;

		//	float4 v2 = mul(view, float4(m_Particles[i].m_Position, 1.f));
		//	float4 pos2 = mul(projection, v2);

		//	float r2 = 2 * tan(1 / (fov * 0.5f)) / v2.z;

		//	float2 win2;
		//	win2.x = round(((v2.x + 1) / 2.0) *
		//		1280);
		//	//we calculate -point3D.getY() because the screen Y axis is
		//	//oriented top->down 
		//	win2.y = round(((1 - v2.y) / 2.0) *
		//		720);

		//	if (isFullyOccluded(win, win2, r1, r2))
		//		return;
		//}

		//float4 viewPos = mul(view, float4(p.m_Position, 1.f));
		//float4 projPos = mul(projection, viewPos);
		//projPos /= projPos.w;
		//float2 uv = float2((projPos.x + 1.0) * 0.5f, (1.0 - projPos.y) * 0.5f);
		//float depth = m_Depth.Load(int3(uv.x, uv.y, 0)).r;

		//float4 vProjectedPos = float4(projPos.x, projPos.y, depth, 1.0f);
		//	// Transform by the inverse projection matrix
		//	float4 vPositionVS = mul(inverseprojection, vProjectedPos);
		//	// Divide by w to get the view-space position
		//	float3 VV = vPositionVS.xyz / vPositionVS.w;

			//if (viewPos.z + 2.f > VV.z)
			//	return;


		Particle part;
		part.m_Position = p.m_Position;
		part.m_Color = p.m_Color;
		part.m_Radius = p.m_Radius;
		m_output.Append(part);
	}
}
#endif