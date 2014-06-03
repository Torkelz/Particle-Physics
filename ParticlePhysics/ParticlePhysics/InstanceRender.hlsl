#pragma pack_matrix(row_major)

cbuffer cb : register(b0)
{
	float4x4	view;
	float4x4	projection;
};

struct VSLightInput
{
	float3	vposition		: POSITION;
	float3	lightPos		: LPOSITION;
    float3	lightColor		: COLOR;
    float	lightRange		: RANGE;
};

struct VSLightOutput
{
	float4	vposition		: SV_Position;
    float3	lightColor		: COLOR;
};

//############################
// Shader step: Vertex Shader
//############################
VSLightOutput PointLightVS(VSLightInput input)
{
	float s = input.lightRange;
	float3 t = input.lightPos;
	float4x4 scale =
	{
		float4(s,0,0,0),
		float4(0,s,0,0),
		float4(0,0,s,0),
		float4(0,0,0,1)
	};
	float4x4 trans =
	{  
		float4(1,0,0,t.x),
		float4(0,1,0,t.y),
		float4(0,0,1,t.z),
		float4(0,0,0,1)
	};

	float4 pos = mul(scale, float4(input.vposition,1.0f));
	pos = mul(trans, pos);

	VSLightOutput output;
	output.vposition		= mul(projection, mul(view, pos));
	output.lightColor		= input.lightColor;
	return output;
}
//############################
// Shader step: Pixel Shader
//############################
float4 PointLightPS(VSLightOutput input) : SV_TARGET
{
	return float4( input.lightColor, 1.0f );
}