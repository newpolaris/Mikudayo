#include "ShaderUtility.hlsli"
#include "PostEffectsRS.hlsli"
#include "PixelPacking.hlsli"

Texture2D<float3> Bloom : register(t0);
#if SUPPORT_TYPED_UAV_LOADS
RWTexture2D<float3> SrcColor : register(u0);
#elif USE_FXAA_PS
RWTexture2D<float3> DstColor : register(u0);
Texture2D<float3> SrcColor : register(t2);
#else
RWTexture2D<uint> DstColor : register(u0);
Texture2D<float3> SrcColor : register(t2);
#endif
RWTexture2D<unorm float> OutLuma : register(u1);
SamplerState LinearSampler : register(s0);

cbuffer Constants : register(b0)
{
    float2 g_RcpBufferDim;
    float g_BloomStrength;
};

[RootSignature(PostEffects_RootSig)]
[numthreads( 8, 8, 1 )]
void main( uint3 DTid : SV_DispatchThreadID )
{
    float2 TexCoord = (DTid.xy + 0.5) * g_RcpBufferDim;

    // Load LDR and bloom
    float3 ldrColor = SrcColor[DTid.xy];
    ldrColor += g_BloomStrength * Bloom.SampleLevel(LinearSampler, TexCoord, 0);

#if SUPPORT_TYPED_UAV_LOADS
    SrcColor[DTid.xy] = ldrColor;
#elif USE_FXAA_PS
    DstColor[DTid.xy] = ldrColor;
#else
    DstColor[DTid.xy] = Pack_R11G11B10_FLOAT(ldrColor);
#endif
    OutLuma[DTid.xy] = RGBToLuminance(ldrColor);
}
