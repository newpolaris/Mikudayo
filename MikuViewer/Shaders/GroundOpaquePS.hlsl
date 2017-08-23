#include "ShadowDefine.hlsli"

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

static const float Bias = 0.f;
#include "Shadow.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float3 shadow = GetShadow(input.shadowPosH, input.posH.xyz);
    return float4(shadow, 1.0f);
}