// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 posH : SV_POSITION;
	float3 posV : POSITION;
	float3 normalV : NORMAL;
	float2 uv : TEXTURE;
};

struct Material
{
	float3 diffuse;
	float alpha; // difuse alpha
	float3 specular;
	float specularPower;
	float3 ambient;
};

struct DirectionalLight
{
	float3 color;
	float pad;
	float3 dirV; // light incident direction I
};

static const int kSphereNone = 0;
static const int kSphereMul = 1;
static const int kSphereAdd = 2;

cbuffer MaterialConstants : register(b0)
{
	Material material;
	int sphereOperation;
	int bUseToon;
};

static const int maxLight = 4;
cbuffer PassConstants : register(b1)
{
	DirectionalLight light[maxLight];
	uint numLight;
}

Texture2D<float4>	texDiffuse		: register(t0);
Texture2D<float3>	texSphere		: register(t1);
Texture2D<float3>	texToon         : register(t2);
SamplerState		sampler0		: register(s0);

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 tex = texDiffuse.Sample( sampler0, input.uv );

	float3 lightVecV = normalize( -light[0].dirV );
	float3 normalV = normalize( input.normalV );
	float intensity = dot( lightVecV, normalV ) * 0.5 + 0.5;
	//
	// Toon texture :
	//
	// For v, from 0.0 to 1.0 which means from bright to dark.
	// For u, can't find valid equation. Usually, it is ignored in various model.
	//
	// http://trackdancer.deviantart.com/art/MMD-PMD-Tutorial-Toon-Shaders-Primer-394445914
	// 
	float2 toonCoord = float2( 0.5, 1.0 - intensity );

	float3 toEyeV = -input.posV;
	float3 halfV = normalize( toEyeV + lightVecV );


	float NdotH = dot( normalV, halfV );
	float specularFactor = pow( max(NdotH, 0.001f), material.specularPower );

	float3 diffuse = material.diffuse * light[0].color;
	float3 ambient = material.ambient;
	float3 specular = specularFactor * material.specular;

	float3 texColor = tex.xyz;

	float2 sphereCoord = 0.5 + 0.5*float2(1.0, -1.0) * normalV.xy;
	if (sphereOperation == kSphereAdd)
		texColor += texSphere.Sample( sampler0, sphereCoord );
	else if (sphereOperation == kSphereMul)
		texColor *= texSphere.Sample( sampler0, sphereCoord );

	float3 color = texColor * (ambient + diffuse) + specular;
	if (bUseToon) 
		color *= texToon.Sample( sampler0, toonCoord );

	float alpha = tex.a * material.alpha;

	return float4(color, alpha);
}
