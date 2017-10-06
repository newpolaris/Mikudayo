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
    float3 NormalVS : SV_Target0;   // View space normal (R11G11B10_FLOAT)
    float4 SpecularPower : SV_Target1; // Specular power (R8_UNORM)
};

Texture2D<float4> texDiffuse : register(t1);
Texture2D<float3> texSphere : register(t2);
Texture2D<float3> texToon : register(t3);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

[earlydepthstencil]
PixelShaderOutput main( PixelShaderInput input )
{
    PixelShaderOutput Out;

    float3 normalVS = normalize( input.normalVS );
    Out.NormalVS = 0.5 * (normalVS + 1);
    Out.SpecularPower = material.specularPower / 255.0;

    return Out;
}