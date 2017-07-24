// Poisson disk kerenl width
#define PoissonKernel_ 5
#include "PCFKernels.hlsli"

// Shadow filter method list
#define ShadowModeSingle_ 0
#define ShadowModeWeighted_ 1
#define ShadowModePoisson_ 2
#define ShadowModePoissonRotated_ 3
#define ShadowModePoissonStratified_ 4

// Shadow filter method
#define ShadowMode_ ShadowModePoissonDiskStratified

struct ShadowTex
{
    SamplerComparisonState smpl;
    texture2D<float> tex;
    float4 ShadowTexelSize;
    float3 Position;
};

//
// Generates pseudorandom number in [0, 1]
// the pseudorandom numbers will change with change of world space position
//
// seed: world space position of a fragemnt
// freq: modifier for seed. The bigger, the faster 
//
// Uses code from "http://www.sunandblackcat.com/tipFullView.php?l=eng&topicid=35"
//
float Random(float3 seed, float freq)
{
    // project seed on random constant vector
    float dt = dot( floor( seed * freq ), float3( 53.1215, 21.1352, 9.1322 ) );
    // return only fractional part
    return frac( sin( dt ) * 2105.2354 );
} 

//
// Returns random angle
//
float RandomAngle(float3 seed, float freq)
{
    return Random(seed, freq) * 6.283285;
}

//
// Single test
//
float SampleSingle( ShadowTex Input, float3 ShadowPos )
{
    return Input.tex.SampleCmpLevelZero( Input.smpl, ShadowPos.xy, ShadowPos.z );
}

//
// Poisson Disk sampling with no weight
//
// Uses code from "MiniEngine"
//
float SampleWeight( ShadowTex Input, float3 ShadowPos )
{
    const float Dilation = 2.0;

    texture2D<float> texShadow = Input.tex;
    SamplerComparisonState shadowSampler = Input.smpl;

    float d1 = Dilation * Input.ShadowTexelSize.x * 0.125;
    float d2 = Dilation * Input.ShadowTexelSize.x * 0.875;
    float d3 = Dilation * Input.ShadowTexelSize.x * 0.625;
    float d4 = Dilation * Input.ShadowTexelSize.x * 0.375;
    return 0.1 * (
        2.0 * texShadow.SampleCmpLevelZero( shadowSampler, ShadowPos.xy, ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPos.xy + float2(-d2, d1), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPos.xy + float2(-d1, -d2), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPos.xy + float2(d2, -d1), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPos.xy + float2(d1, d2), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPos.xy + float2(-d4, d3), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPos.xy + float2(-d3, -d4), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPos.xy + float2(d4, -d3), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowPos.xy + float2(d3, d4), ShadowPos.z ) );
}

//
// Poisson Disk sampling with no weight
//
float SamplePoissonDisk( ShadowTex Input, float3 ShadowPos )
{
    texture2D<float> texShadow = Input.tex;
    SamplerComparisonState shadowSampler = Input.smpl;
    float2 Texel = Input.ShadowTexelSize.xx;

    float Result = 0.0;
    for (int i = 0; i < kPoissonSample; i++)
        if (texShadow.SampleCmpLevelZero( shadowSampler, ShadowPos.xy + PoissonDisk[i] * Texel, ShadowPos.z ))
            Result += 1.0;
    return Result / kPoissonSample;
}

//
// Poisson Disk Rotated sampling
//
// Uses code from "http://www.sunandblackcat.com/tipFullView.php?l=eng&topicid=35"
//
float SamplePoissonDiskRotated( ShadowTex Input, float3 ShadowPos )
{
    const float PCFRadius = 1.5;
    const float Freq = 15;

    texture2D<float> texShadow = Input.tex;
    SamplerComparisonState shadowSampler = Input.smpl;
    float2 Texel = Input.ShadowTexelSize.xy;

    float Result = 0.0;
    float theta = RandomAngle( Input.Position.xyz, Freq );
    float S = sin( theta );
    float C = cos( theta );
    float2x2 Rotation = float2x2(
        float2( C, -S ),
        float2( S,  C ));

    for (int i = 0; i < kPoissonSample; i++)
    {
        float2 Pos = ShadowPos.xy + mul(PoissonDisk[i], Rotation)*Texel*PCFRadius;
        if (texShadow.SampleCmpLevelZero( shadowSampler, Pos, ShadowPos.z ))
            Result += 1.0;
    }
    return Result / kPoissonSample;
}

//
// Poisson Disk Rotated sampling
//
// Uses code from "http://www.sunandblackcat.com/tipFullView.php?l=eng&topicid=35"
//
float ShadowModePoissonDiskStratified( ShadowTex Input, float3 ShadowPos )
{
    texture2D<float> texShadow = Input.tex;
    SamplerComparisonState shadowSampler = Input.smpl;
    float2 Texel = Input.ShadowTexelSize.xy;

    float Result = 0.0;
    uint Num = kPoissonSample/2;
    for (uint i = 0; i < Num; i++) {
        uint index = uint(kPoissonSample*Random( Input.Position.xyz, i )) % kPoissonSample;
        Result += texShadow.SampleCmpLevelZero( shadowSampler, ShadowPos.xy + PoissonDisk[index]*Texel, ShadowPos.z );
    }
    return Result / Num; 
}

float GetShadow( ShadowTex Input, float4 ShadowPos )
{
    // Complete projection by doing division by w.
    ShadowPos.xyz /= ShadowPos.w;

    float2 TransTexCoord = ShadowPos.xy;
    float Result = 1.f;
    if (!any(saturate(TransTexCoord) != TransTexCoord)) {

#if ShadowMode_ == ShadowModeSingle_
        Result = SampleSingle( Input, ShadowPos.xyz );
#elif ShadowMode_ == ShadowModeWeighted_
        Result = SampleWeighted( Input, ShadowPos.xyz );
#elif ShadowMode_ == ShadowModePoisson_
        Result = SamplePoissonDisk( Input, ShadowPos.xyz );
#elif ShadowMode_ == ShadowModePoissonRotated_
        Result = SamplePoissonDiskRotated( Input, ShadowPos.xyz );
#else // if ShadowMode_ == ShadowModePoissonStratified_
        Result = ShadowModePoissonDiskStratified( Input, ShadowPos.xyz );
#endif
    }
    return Result * 0.5 + 0.5;
}

