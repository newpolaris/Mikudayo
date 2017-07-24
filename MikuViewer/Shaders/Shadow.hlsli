// #define SINGLE_SAMPLE
#define POISSON 1
#define POISSON9 1

#if POISSON4
static const int kPoissonSample = 4;
float2 PoissonDisk[kPoissonSample] = {
    float2(-0.94201624, -0.39906216),
    float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870),
    float2(0.34495938, 0.29387760)
};
#elif POISSON9
static const int kPoissonSample = 9;
float2 PoissonDisk[kPoissonSample] = {
    float2(0.07881197f, 0.03746713f),
    float2(0.2480896f, -0.8793799f),
    float2(0.6023098f, 0.2738653f),
    float2(-0.3207254f, 0.5149536f),
    float2(0.5588677f, -0.2816212f),
    float2(-0.4454237f, -0.2362587f),
    float2(0.1127232f, 0.8650647f),
    float2(-0.4380007f, -0.799684f),
    float2(-0.9074062f, 0.1835023)
};
#elif POISSON16
static const int kPoissonSample = 16;
float2 PoissonDisk[kPoissonSample] = {
   float2( -0.94201624, -0.39906216 ), 
   float2( 0.94558609, -0.76890725 ), 
   float2( -0.094184101, -0.92938870 ), 
   float2( 0.34495938, 0.29387760 ), 
   float2( -0.91588581, 0.45771432 ), 
   float2( -0.81544232, -0.87912464 ), 
   float2( -0.38277543, 0.27676845 ), 
   float2( 0.97484398, 0.75648379 ), 
   float2( 0.44323325, -0.97511554 ), 
   float2( 0.53742981, -0.47373420 ), 
   float2( -0.26496911, -0.41893023 ), 
   float2( 0.79197514, 0.19090188 ), 
   float2( -0.24188840, 0.99706507 ), 
   float2( -0.81409955, 0.91437590 ), 
   float2( 0.19984126, 0.78641367 ), 
   float2( 0.14383161, -0.14100790 ) 
};

#endif

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

    float2 TransTexCoord = ShadowPosH.xy;
    float2 Texel = Input.ShadowTexelSize.xx;
    float Result = 1.f;
    if (!any(saturate(TransTexCoord) != TransTexCoord)) {
#ifdef SINGLE_SAMPLE
	    Result = Input.tex.SampleCmpLevelZero(Input.smpl, ShadowPosH.xy, ShadowPosH.z);
#elif POISSON
        texture2D<float> texShadow = Input.tex;
        SamplerComparisonState shadowSampler = Input.smpl;
        Result = 0.0;
        for (int i = 0; i < kPoissonSample; i++)
            if (texShadow.SampleCmpLevelZero( shadowSampler, ShadowPosH.xy + PoissonDisk[i]*Texel, ShadowPosH.z )) 
                Result += 1.0;
        Result /= kPoissonSample;
#else
        const float Dilation = 2.0;
        float d1 = Dilation * Input.ShadowTexelSize.x * 0.125;
        float d2 = Dilation * Input.ShadowTexelSize.x * 0.875;
        float d3 = Dilation * Input.ShadowTexelSize.x * 0.625;
        float d4 = Dilation * Input.ShadowTexelSize.x * 0.375;
        texture2D<float> texShadow = Input.tex;
        SamplerComparisonState shadowSampler = Input.smpl;
        Result = (
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
    }
    return Result * Result * 0.5 + 0.5;
}

