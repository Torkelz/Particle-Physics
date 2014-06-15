#pragma pack_matrix(row_major)

cbuffer cb : register(b0)
{
	float4x4	view;
	float4x4	projection;
};

struct VSLightInput
{
	float3	vposition	: POSITION;
	float3	lightPos	: LPOSITION;
    float3	lightColor	: COLOR;
    float	lightRange	: RANGE;
};

struct VSLightOutput
{
	float4	vposition	: SV_Position;
    float3	lightColor	: COLOR;
	float3	normal		: NORMAL;
	float3	wpos		: WPOS;
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
	output.normal = normalize(mul(view, float4(pos - input.lightPos, 0.f)).xyz);
	output.wpos = pos;
	return output;
}
//############################
// Shader step: Pixel Shader
//############################
float4 PointLightPS(VSLightOutput input) : SV_TARGET
{
			float3 light = float3(0,0,50);

			float3 litColor = 0;
			//The vector from surface to the light
			float3 lightVec = input.wpos - light;
			float lightintensity;
			float3 lightDir;
			float3 reflection;
			float3 specular = 1;
			float3 ambient = float3(0.5,0.5,0.5);
			float3 diffuse = float3(0.4,0.4,0.4);
			float shininess = 32;

			//the distance deom surface to light
			float d = length(lightVec);
			float fade;
			if(d > 500)
					return float4(float3(0.5f, 0.5f, 0.5f) * input.lightColor,1);
			fade = 1 - (d/ 500);
			//Normalize light vector
			lightVec /= d;
			litColor = ambient.xyz;
			//Add ambient light term
        
			lightintensity = saturate(dot(input.normal, lightVec));
			litColor += diffuse.xyz * lightintensity;
			lightDir = -lightVec;
			if(lightintensity > 0.0f)
			{
				float3 viewDir = normalize(light - float3(0,0,50));
				float3 ref = reflect(-lightDir, normalize(input.normal));
				float scalar = max(dot(ref, viewDir), 0.0f);
				float specFac = 1.0f;
				for(int i = 0; i < shininess;i++)
					specFac *= scalar;
				litColor += specular.xyz * specFac;
			}
			litColor = litColor * input.lightColor;

			return float4(litColor*fade,1);

	return float4( input.lightColor, 1.0f );
}