#include "CommonInclude.hlsli"

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 posHS : SV_POSITION;
	float3 posVS : POSITION0;
	float3 normalVS : NORMAL;
	float2 uv : TEXTURE;
};

struct PixelShaderOutput
{
    float4 LightAccumulation    : SV_Target0;   // Ambient + emissive (R8G8B8_????) Unused (A8_UNORM)
    float4 Diffuse              : SV_Target1;   // Diffuse Albedo (R8G8B8_UNORM) Unused (A8_UNORM)
    float4 Specular             : SV_Target2;   // Specular Color (R8G8B8_UNROM) Specular Power(A8_UNORM)
    float4 NormalVS             : SV_Target3;   // View space normal (R32G32B32_FLOAT) Unused (A32_FLOAT)
};

static const int kSphereNone = 0;
static const int kSphereMul = 1;
static const int kSphereAdd = 2;

cbuffer MaterialConstants : register(b0)
{
	Material material;
	int sphereOperation;
	int bUseTexture;
	int bUseToon;
};

cbuffer PassConstants : register(b1)
{
    float3 SunDirectionVS;
    float3 SunColor;
    float4 ShadowTexelSize;
}

Texture2D<float4> texDiffuse : register(t1);
Texture2D<float3> texSphere : register(t2);
Texture2D<float3> texToon : register(t3);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

PixelShaderOutput main( PixelShaderInput input )
{
    PixelShaderOutput Out;

	float3 lightVecVS = normalize( -SunDirectionVS );
	float3 normalVS = normalize( input.normalVS );
	float intensity = dot( lightVecVS, normalVS ) * 0.5 + 0.5;
	float2 toonCoord = float2(0.5, 1.0 - intensity);

    float3 diffuse = material.diffuse;
    float3 ambient = material.ambient;

	float3 texColor = float3(1.0, 1.0, 1.0);
	if (bUseTexture)
	{
		float4 tex = texDiffuse.Sample( sampler0, input.uv );
		texColor = tex.xyz;
	}

	float2 sphereCoord = 0.5 + 0.5*float2(1.0, -1.0) * normalVS.xy;
	if (sphereOperation == kSphereAdd)
		texColor += texSphere.Sample( sampler0, sphereCoord );
	else if (sphereOperation == kSphereMul)
		texColor *= texSphere.Sample( sampler0, sphereCoord );
    if (bUseToon)
        texColor *= texToon.Sample( sampler0, toonCoord );

    Out.LightAccumulation = float4(texColor * ambient, 1.0);
    Out.Diffuse = float4(texColor * diffuse, 1.0);
    Out.NormalVS = float4(normalVS, 0);
    // Method of packing specular power from "Deferred Rendering in Killzone 2" presentation 
    // from Michiel van der Leeuw, Guerrilla (2007)
    Out.Specular = float4( material.specular.rgb, log2( material.specularPower ) / 10.5f ); 

    return Out;
}