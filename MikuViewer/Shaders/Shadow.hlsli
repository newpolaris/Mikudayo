// #define SINGLE_SAMPLE
static const int PCF_SIZE = 2;

struct ShadowTex
{
    SamplerComparisonState smpl;
    texture2D<float> tex;
    float4 ShadowTexelSize;
};

float GetShadow(ShadowTex Input, float4 ShadowPosH)
{
    // Complete projection by doing division by w.
    ShadowPosH.xyz /= ShadowPosH.w;

#ifdef SINGLE_SAMPLE
	float result = Input.tex.SampleCmpLevelZero(Input.smpl, ShadowPosH.xy, ShadowPosH.z);
#else
    const float Dilation = 2.0;
    float d1 = Dilation * Input.ShadowTexelSize.x * 0.125;
    float d2 = Dilation * Input.ShadowTexelSize.x * 0.875;
    float d3 = Dilation * Input.ShadowTexelSize.x * 0.625;
    float d4 = Dilation * Input.ShadowTexelSize.x * 0.375;
    texture2D<float> texShadow = Input.tex;
    SamplerComparisonState shadowSampler = Input.smpl;
    float result = (
        2.0 * texShadow.SampleCmpLevelZero( shadowSampler, ShadowPosH.xy, ShadowPosH.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPosH.xy + float2(-d2,  d1), ShadowPosH.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPosH.xy + float2(-d1, -d2), ShadowPosH.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPosH.xy + float2( d2, -d1), ShadowPosH.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPosH.xy + float2( d1,  d2), ShadowPosH.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPosH.xy + float2(-d4,  d3), ShadowPosH.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPosH.xy + float2(-d3, -d4), ShadowPosH.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPosH.xy + float2( d4, -d3), ShadowPosH.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPosH.xy + float2( d3,  d4), ShadowPosH.z )
        ) / 10.0;
#endif
    return result * result;
}

