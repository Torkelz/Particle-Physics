#pragma pack_matrix(row_major)
SamplerState m_textureSampler : register(s0);
Texture2D m_texture : register(t0);

struct VSIn
{
	float3 position : POSITION;
	float3 color : COLOR;
	float radius : RADIUS;
};
struct GSIn
{
	float3 position : POSITION;
	float4 color : COLOR;
	float radius : RADIUS;
};
//cbuffer b0 : register(c0)
//{
//	float4x4 projection;
//};
//cbuffer b1 : register(c1)
//{
//	float4x4 view;
//	float4 eyePosW;
//};
cbuffer cb : register(c0)
{
	float4x4	view;
	float4x4	projection;
	float4x4	inverseprojection;
};

struct PSIn
{
	float4 position : SV_POSITION;
	float3 wposition : POSITION;
	float radius : RADIUS;
	float4 color : COLOR;
	float3 up : UP;
	float2 uvCoord : COORD;
};
GSIn VS(VSIn input)
{
	GSIn res =
	{
		input.position,
		float4(input.color, 1.f),
		input.radius
	};
	return res;
}
// The draw GS just expands points into camera facing quads.
[maxvertexcount(4)]
void GS(point GSIn gIn[1], inout TriangleStream<PSIn> triStream)
{
	//compute world matrix so that billboard faces the camera
	float3 t_position = gIn[0].position;

	float3 look = normalize(float3(0, 0, 50) - t_position);
	float3 right = normalize(cross(float3(0, 1, 0), look));
	float3 up = cross(look, right);

	float4x4 world = { float4(-right.x, up.x, look.x, t_position.x),
	float4(-right.y, up.y, look.y, t_position.y),
	float4(-right.z, up.z, look.z, t_position.z),
	float4(0, 0, 0, 1) };

	float4x4 WVP = mul(projection, mul(view, world));
		//compute 4 triangle strip vertices (quad) in local space.
		//the quad faces down the +Z axis in local space.
	float4 v[4];
	v[0] = float4(-gIn[0].radius, -gIn[0].radius, 0.0f, 1.0f);
	v[1] = float4(+gIn[0].radius, -gIn[0].radius, 0.0f, 1.0f);
	v[2] = float4(-gIn[0].radius, +gIn[0].radius, 0.0f, 1.0f);
	v[3] = float4(+gIn[0].radius, +gIn[0].radius, 0.0f, 1.0f);
	//tansform quad vertices to world space and output them as a triangle strip.
	PSIn gOut;
	//[unroll]
	float2 quadUVC[] =
	{
		float2(0.f, 1.f),
		float2(1.f, 1.f),
		float2(0.f, 0.f),
		float2(1.f, 0.f),
	};

	for (int i = 0; i < 4; i++)
	{
		gOut.position = mul(WVP, v[i]);
		gOut.uvCoord = quadUVC[i];
		gOut.color = gIn[0].color;
		gOut.wposition = t_position;
		gOut.up = up;
		gOut.radius = gIn[0].radius;
		triStream.Append(gOut);
	}
}
//http://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_shading_model
struct Lighting
{
	float3 Diffuse;
	float3 Specular;
};

struct PointLight
{
	float3 position;
	float3 diffuseColor;
	float  diffusePower;
	float3 specularColor;
	float  specularPower;
};

Lighting GetPointLight(PointLight light, float3 pos3D, float3 viewDir, float3 normal)
{
	Lighting OUT;
	if (light.diffusePower > 0)
	{
		float3 lightDir = light.position - pos3D; //3D position in space of the surface
			float distance = length(lightDir);
		lightDir = lightDir / distance; // = normalize( lightDir );
		distance = distance * distance; //This line may be optimised using Inverse square root

		//Intensity of the diffuse light. Saturate to keep within the 0-1 range.
		float NdotL = dot(normal, lightDir);
		float intensity = saturate(NdotL);

		// Calculate the diffuse light factoring in light color, power and the attenuation
		OUT.Diffuse = intensity *light.diffuseColor * light.diffusePower / distance;

		//Calculate the half vector between the light vector and the view vector.
		//This is faster than calculating the actual reflective vector.
		float3 H = normalize(lightDir + viewDir);

			//Intensity of the specular light
			float NdotH = dot(normal, H);
		intensity = pow(saturate(NdotH), 1);

		//Sum up the specular light factoring
		OUT.Specular = intensity * light.specularColor * light.specularPower / distance;
	}
	return OUT;
}

float4 PS(PSIn p_input) : SV_TARGET
{
	float4 temp = m_texture.Sample(m_textureSampler, p_input.uvCoord);
	if (temp.w < 0.2f)
		discard;

	//return float4(p_input.color.xyz, temp.w);

	float3 look = normalize(float3(0, 0, 50) - p_input.wposition);
	float radius = p_input.radius;
	float3 right = normalize(cross(float3(0, 1, 0), look));
		float3 up = cross(look, right);

		float3 lookPos = (temp.x*radius) * look;
		float3 uvPosX = right * (((p_input.uvCoord.x * 2) - 1) * radius);
		float3 uvPosY = up * (((p_input.uvCoord.y * 2) - 1) * radius);

		float3 endPos = lookPos + uvPosX + uvPosY;

		float3 normal = normalize(endPos);
		float3 surfacePosition = endPos + p_input.wposition;

		PointLight pl;
	pl.position = float3(0, 0, 0);
	pl.diffuseColor = float3(0.4f, 0.4f, 0.4f);
	pl.diffusePower = 200;
	pl.specularColor = float3(1, 1, 1);;
	pl.specularPower = 100;

	//Lighting L = GetPointLight(pl, endPos + p_input.wposition, pl.position - float3(0, 0, 50), normal);

	//return float4(L.Diffuse * p_input.color, temp.w);

	//// Compute the half vector
	//float3 half_vector = normalize((pl.position - surfacePosition) + (float3(0, 0, 50) - surfacePosition));

	//// Compute the angle between the half vector and normal
	//float  HdotN = max(0.0f, dot(half_vector, normal));

	//// Compute the specular colour
	//float3 specular = pl.specularColor * pow(HdotN, 2);

	//	// Compute the diffuse term
	//	float3 diffuse = pl.diffuseColor * max(0.0f, dot(normal, normalize(pl.position - surfacePosition)));

	//	// Determine the final colour
	//return float4((diffuse + specular) * p_input.color, temp.w);


	float3 L = pl.position - surfacePosition;
	float dist = length(L);
	//float attenuation = max(0.f, 1.f - (dist / 1000));
	L /= dist;
	
	//if (attenuation == 0.f)
	//	return float4(float3(0.2f, 0.2f, 0.2f) *p_input.color, temp.w);

	float nDotL = saturate(dot(normal, L));
	float3 diffuse = nDotL *pl.diffuseColor * p_input.color;
		// Final value is the sum of the albedo and diffuse with attenuation applied
		return float4(saturate(diffuse + float3(0.2f, 0.2f, 0.2f) *p_input.color), temp.w);
}