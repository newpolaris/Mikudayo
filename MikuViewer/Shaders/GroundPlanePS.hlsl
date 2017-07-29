#include "Shadow.hlsli"

struct PixelShaderInput
{
	float4 posH : SV_POSITION;
    float4 shadowPosH[MaxSplit] : POSITION0;
};

cbuffer PassConstants : register(b1)
{
    float3 SunDirection;
    float3 SunColor;
    float4 ShadowTexelSize;
}

Texture2DArray<float> texShadow : register(t4);
SamplerComparisonState samplerShadow    : register(s2);

float4 main(PixelShaderInput input) : SV_TARGET
{
    ShadowTex tex = { ShadowTexelSize, input.posH.xyz, 0, texShadow, samplerShadow };
	float shadow = GetShadow(tex, input.shadowPosH);
    return float4(shadow.xxx, 1.0f);
}