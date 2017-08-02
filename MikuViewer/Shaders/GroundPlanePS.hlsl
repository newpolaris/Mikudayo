#include "Shadow.hlsli"

struct PixelShaderInput
{
	float4 posH : SV_POSITION;
    float3 posW : POSITION1;
	float3 normalW : NORMAL1;
    float depthV : DEPTH0;
};

cbuffer PassConstants : register(b1)
{
    float3 SunDirection;
    float3 SunColor;
    float4 ShadowTexelSize;
    matrix ShadowMatrix;
    float4 CascadeOffsets[kShadowSplit];
    float4 CascadeScales[kShadowSplit];
    float CascadeSplits[kShadowSplit];
}

Texture2DArray<float> texShadow : register(t4);
SamplerComparisonState samplerShadow    : register(s2);

float4 main(PixelShaderInput input) : SV_TARGET
{
    ShadowTex tex = { ShadowTexelSize, input.posH.xyz, 0, texShadow, samplerShadow, ShadowMatrix, CascadeOffsets, CascadeScales, CascadeSplits };
    uint2 screenPos = uint2(input.posH.xy);
	float3 shadowVisibility = ShadowVisibility(tex, input.posW, input.depthV, 1.f, float3(0, 1, 0), screenPos);
    return float4(shadowVisibility, 1.0f);
}