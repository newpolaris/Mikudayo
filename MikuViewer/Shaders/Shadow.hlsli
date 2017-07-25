//
// Poisson disk kerenl width
//
#define PoissonKernel_ 4

//
// 2, 3, 5, 7
//
#define FilterSize_ 7

#include "PCFKernels.hlsli"

// Shadow filter method list
#define ShadowModeSingle_ 0
#define ShadowModeWeighted_ 1
#define ShadowModePoisson_ 2
#define ShadowModePoissonRotated_ 3
#define ShadowModePoissonStratified_ 4
#define ShadowModeOptimizedGaussinPCF_ 5
#define ShadowModeFixedSizePCF_ 6

// Shadow filter method
#define ShadowMode_ ShadowModePoisson_

struct ShadowTex
{
    float4 ShadowTexelSize;
    float3 Position;
    float Bias;
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
    return texShadow.SampleCmpLevelZero( samplerShadow, ShadowPos.xy, ShadowPos.z );
}

//
// 3 tap sized sampling method with total 9 sampling point
//
// Uses code from "MiniEngine"
//
float SampleWeighted( ShadowTex Input, float3 ShadowPos )
{
    const float Dilation = 2.0;

    float d1 = Dilation * Input.ShadowTexelSize.x * 0.125;
    float d2 = Dilation * Input.ShadowTexelSize.x * 0.875;
    float d3 = Dilation * Input.ShadowTexelSize.x * 0.625;
    float d4 = Dilation * Input.ShadowTexelSize.x * 0.375;
    return 0.1 * (
        2.0 * texShadow.SampleCmpLevelZero( samplerShadow, ShadowPos.xy, ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( samplerShadow, ShadowPos.xy + float2(-d2, d1), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( samplerShadow, ShadowPos.xy + float2(-d1, -d2), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( samplerShadow, ShadowPos.xy + float2(d2, -d1), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( samplerShadow, ShadowPos.xy + float2(d1, d2), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( samplerShadow, ShadowPos.xy + float2(-d4, d3), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( samplerShadow, ShadowPos.xy + float2(-d3, -d4), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( samplerShadow, ShadowPos.xy + float2(d4, -d3), ShadowPos.z ) +
        texShadow.SampleCmpLevelZero( samplerShadow, ShadowPos.xy + float2(d3, d4), ShadowPos.z ) );
}

//
// Poisson Disk sampling with no weight
//
float SamplePoissonDisk( ShadowTex Input, float3 ShadowPos )
{
    float2 Texel = Input.ShadowTexelSize.xx;

    float Result = 0.0;
    for (int i = 0; i < kPoissonSample; i++)
        if (texShadow.SampleCmpLevelZero( samplerShadow, ShadowPos.xy + PoissonDisk[i] * Texel, ShadowPos.z ))
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
    const float PCFRadius = 1.1;
    const float Freq = 15;
    const float SampleDivFactor = 1.0;
    const float2 Texel = Input.ShadowTexelSize.xy;

    float Result = 0.0;
    float theta = RandomAngle( Input.Position.xyz, Freq );
    float S = sin( theta );
    float C = cos( theta );
    float2x2 Rotation = float2x2(
        float2( C, -S ),
        float2( S,  C ));

    uint Num = kPoissonSample/SampleDivFactor;
    for (uint i = 0; i < Num; i++)
    {
        float2 Pos = ShadowPos.xy + mul(PoissonDisk[i], Rotation)*Texel*PCFRadius;
        if (texShadow.SampleCmpLevelZero( samplerShadow, Pos, ShadowPos.z ))
            Result += 1.0;
    }
    return Result / Num;
}

//
// Poisson Disk Rotated sampling
//
// Uses code from "http://www.sunandblackcat.com/tipFullView.php?l=eng&topicid=35"
//
float ShadowModePoissonDiskStratified( ShadowTex Input, float3 ShadowPos )
{
    float2 Texel = Input.ShadowTexelSize.xy;

    float Result = 0.0;
    uint Num = PoissonKernel_;
    for (uint i = 0; i < Num; i++) {
        uint index = uint(kPoissonSample*Random( Input.Position.xyz, i )) % kPoissonSample;
        Result += texShadow.SampleCmpLevelZero( samplerShadow, ShadowPos.xy + PoissonDisk[index]*Texel, ShadowPos.z );
    }
    return Result / Num; 
}

//
// Helper function for SampletexShadowOptimizedGaussinPCF
//
float SampletexShadow(float2 base_uv, float u, float v, float2 texShadowSizeInv,
                      uint cascadeIdx, float depth, float2 receiverPlaneDepthBias) 
{

    float2 uv = base_uv + float2(u, v) * texShadowSizeInv;

    #if UsePlaneDepthBias_
        float z = depth + dot(float2(u, v) * texShadowSizeInv, receiverPlaneDepthBias);
    #else
        float z = depth;
    #endif
    return texShadow.SampleCmpLevelZero(samplerShadow, uv, z);
}

//
// The method used in The Witness
//
// Optimized with linear filtering use same idea as 'Fast gaussion blur with linear filtering'
// But, to apply in PS shader it use x,y bilinear filtering
//
// Uses code from "https://github.com/TheRealMJP/Shadows" by MJP
//
float SampletexShadowOptimizedGaussinPCF( ShadowTex Input, float3 ShadowPos, float3 shadowPosDX, float3 shadowPosDY, uint cascadeIdx ) 
{
    float2 texShadowSize;
    float numSlices;
    texShadow.GetDimensions(0, texShadowSize.x, texShadowSize.y, numSlices);

    float lightDepth = ShadowPos.z;
    const float bias = Input.Bias;

    #if UsePlaneDepthBias_
        float2 texelSize = 1.0f / texShadowSize;

        float2 receiverPlaneDepthBias = ComputeReceiverPlaneDepthBias(shadowPosDX, shadowPosDY);

        // Static depth biasing to make up for incorrect fractional sampling on the shadow map grid
        float fractionalSamplingError = 2 * dot(float2(1.0f, 1.0f) * texelSize, abs(receiverPlaneDepthBias));
        lightDepth -= min(fractionalSamplingError, 0.01f);
    #else
        float2 receiverPlaneDepthBias;
        lightDepth -= bias;
    #endif

    float2 uv = ShadowPos.xy * texShadowSize; // 1 unit - 1 texel
    float2 texShadowSizeInv = 1.0 / texShadowSize;

    float2 base_uv;
    base_uv.x = floor(uv.x + 0.5);
    base_uv.y = floor(uv.y + 0.5);

    float s = (uv.x + 0.5 - base_uv.x);
    float t = (uv.y + 0.5 - base_uv.y);

    base_uv -= float2(0.5, 0.5);
    base_uv *= texShadowSizeInv;

    float sum = 0;

    #if FilterSize_ == 2
        return texShadow.SampleCmpLevelZero(samplerShadow, ShadowPos.xy, lightDepth);
    #elif FilterSize_ == 3

        float uw0 = (3 - 2 * s);
        float uw1 = (1 + 2 * s);

        float u0 = (2 - s) / uw0 - 1;
        float u1 = s / uw1 + 1;

        float vw0 = (3 - 2 * t);
        float vw1 = (1 + 2 * t);

        float v0 = (2 - t) / vw0 - 1;
        float v1 = t / vw1 + 1;

        sum += uw0 * vw0 * SampletexShadow(base_uv, u0, v0, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw0 * SampletexShadow(base_uv, u1, v0, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw0 * vw1 * SampletexShadow(base_uv, u0, v1, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw1 * SampletexShadow(base_uv, u1, v1, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        return sum * 1.0f / 16;

    #elif FilterSize_ == 5

        float uw0 = (4 - 3 * s);
        float uw1 = 7;
        float uw2 = (1 + 3 * s);

        float u0 = (3 - 2 * s) / uw0 - 2;
        float u1 = (3 + s) / uw1;
        float u2 = s / uw2 + 2;

        float vw0 = (4 - 3 * t);
        float vw1 = 7;
        float vw2 = (1 + 3 * t);

        float v0 = (3 - 2 * t) / vw0 - 2;
        float v1 = (3 + t) / vw1;
        float v2 = t / vw2 + 2;

        sum += uw0 * vw0 * SampletexShadow(base_uv, u0, v0, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw0 * SampletexShadow(base_uv, u1, v0, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw0 * SampletexShadow(base_uv, u2, v0, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw1 * SampletexShadow(base_uv, u0, v1, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw1 * SampletexShadow(base_uv, u1, v1, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw1 * SampletexShadow(base_uv, u2, v1, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw2 * SampletexShadow(base_uv, u0, v2, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw2 * SampletexShadow(base_uv, u1, v2, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw2 * SampletexShadow(base_uv, u2, v2, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        return sum * 1.0f / 144;

    #else // FilterSize_ == 7

        float uw0 = (5 * s - 6);
        float uw1 = (11 * s - 28);
        float uw2 = -(11 * s + 17);
        float uw3 = -(5 * s + 1);

        float u0 = (4 * s - 5) / uw0 - 3;
        float u1 = (4 * s - 16) / uw1 - 1;
        float u2 = -(7 * s + 5) / uw2 + 1;
        float u3 = -s / uw3 + 3;

        float vw0 = (5 * t - 6);
        float vw1 = (11 * t - 28);
        float vw2 = -(11 * t + 17);
        float vw3 = -(5 * t + 1);

        float v0 = (4 * t - 5) / vw0 - 3;
        float v1 = (4 * t - 16) / vw1 - 1;
        float v2 = -(7 * t + 5) / vw2 + 1;
        float v3 = -t / vw3 + 3;

        sum += uw0 * vw0 * SampletexShadow(base_uv, u0, v0, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw0 * SampletexShadow(base_uv, u1, v0, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw0 * SampletexShadow(base_uv, u2, v0, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw3 * vw0 * SampletexShadow(base_uv, u3, v0, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw1 * SampletexShadow(base_uv, u0, v1, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw1 * SampletexShadow(base_uv, u1, v1, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw1 * SampletexShadow(base_uv, u2, v1, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw3 * vw1 * SampletexShadow(base_uv, u3, v1, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw2 * SampletexShadow(base_uv, u0, v2, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw2 * SampletexShadow(base_uv, u1, v2, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw2 * SampletexShadow(base_uv, u2, v2, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw3 * vw2 * SampletexShadow(base_uv, u3, v2, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw3 * SampletexShadow(base_uv, u0, v3, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw3 * SampletexShadow(base_uv, u1, v3, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw3 * SampletexShadow(base_uv, u2, v3, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw3 * vw3 * SampletexShadow(base_uv, u3, v3, texShadowSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        return sum * 1.0f / 2704;

    #endif
}

//
// Samples the shadow map with a fixed-size PCF kernel optimized with GatherCmp. 
//
// Uses code from "Fast Conventional Shadow Filtering" by Holger Gruen, in GPU Pro.
//
float SampleShadowMapFixedSizePCF(ShadowTex Input, float3 shadowPos, float3 shadowPosDX, float3 shadowPosDY, uint cascadeIdx) 
{
    float2 texShadowSize;
    float numSlices;
    texShadow.GetDimensions(0, texShadowSize.x, texShadowSize.y, numSlices);

    float lightDepth = shadowPos.z;

    const float bias = Input.Bias;

    #if UsePlaneDepthBias_
        float2 texelSize = 1.0f / texShadowSize;

        float2 receiverPlaneDepthBias = ComputeReceiverPlaneDepthBias(shadowPosDX, shadowPosDY);

        // Static depth biasing to make up for incorrect fractional sampling on the shadow map grid
        float fractionalSamplingError = dot(float2(1.0f, 1.0f) * texelSize, abs(receiverPlaneDepthBias));
        lightDepth -= min(fractionalSamplingError, 0.01f);
    #else
        lightDepth -= bias;
    #endif

    #if FilterSize_ == 2
        return texShadow.SampleCmpLevelZero(samplerShadow, float3(shadowPos.xy, cascadeIdx), lightDepth);
    #else
        const int FS_2 = FilterSize_ / 2;

        float2 tc = shadowPos.xy;

        float4 s = 0.0f;
        float2 stc = (texShadowSize * tc.xy) + float2(0.5f, 0.5f);
        float2 tcs = floor(stc);
        float2 fc;
        int row;
        int col;
        float w = 0.0f;
        float4 v1[FS_2 + 1];
        float2 v0[FS_2 + 1];

        fc.xy = stc - tcs;
        tc.xy = tcs / texShadowSize;

        for(row = 0; row < FilterSize_; ++row)
            for(col = 0; col < FilterSize_; ++col)
                w += W[row][col];

        // -- loop over the rows
        [unroll]
        for(row = -FS_2; row <= FS_2; row += 2)
        {
            [unroll]
            for(col = -FS_2; col <= FS_2; col += 2)
            {
                float value = W[row + FS_2][col + FS_2];

                if(col > -FS_2)
                    value += W[row + FS_2][col + FS_2 - 1];

                if(col < FS_2)
                    value += W[row + FS_2][col + FS_2 + 1];

                if(row > -FS_2) {
                    value += W[row + FS_2 - 1][col + FS_2];

                    if(col < FS_2)
                        value += W[row + FS_2 - 1][col + FS_2 + 1];

                    if(col > -FS_2)
                        value += W[row + FS_2 - 1][col + FS_2 - 1];
                }

                if(value != 0.0f)
                {
                    float sampleDepth = lightDepth;

                    #if UsePlaneDepthBias_
                        // Compute offset and apply planar depth bias
                        float2 offset = float2(col, row) * texelSize;
                        sampleDepth += dot(offset, receiverPlaneDepthBias);
                    #endif

                    v1[(col + FS_2) / 2] = texShadow.GatherCmp(samplerShadow, tc.xy,
                                                                 sampleDepth, int2(col, row));
                }
                else
                    v1[(col + FS_2) / 2] = 0.0f;

                if(col == -FS_2)
                {
                    s.x += (1.0f - fc.y) * (v1[0].w * (W[row + FS_2][col + FS_2]
                                         - W[row + FS_2][col + FS_2] * fc.x)
                                         + v1[0].z * (fc.x * (W[row + FS_2][col + FS_2]
                                         - W[row + FS_2][col + FS_2 + 1.0f])
                                         + W[row + FS_2][col + FS_2 + 1]));
                    s.y += fc.y * (v1[0].x * (W[row + FS_2][col + FS_2]
                                         - W[row + FS_2][col + FS_2] * fc.x)
                                         + v1[0].y * (fc.x * (W[row + FS_2][col + FS_2]
                                         - W[row + FS_2][col + FS_2 + 1])
                                         +  W[row + FS_2][col + FS_2 + 1]));
                    if(row > -FS_2)
                    {
                        s.z += (1.0f - fc.y) * (v0[0].x * (W[row + FS_2 - 1][col + FS_2]
                                               - W[row + FS_2 - 1][col + FS_2] * fc.x)
                                               + v0[0].y * (fc.x * (W[row + FS_2 - 1][col + FS_2]
                                               - W[row + FS_2 - 1][col + FS_2 + 1])
                                               + W[row + FS_2 - 1][col + FS_2 + 1]));
                        s.w += fc.y * (v1[0].w * (W[row + FS_2 - 1][col + FS_2]
                                            - W[row + FS_2 - 1][col + FS_2] * fc.x)
                                            + v1[0].z * (fc.x * (W[row + FS_2 - 1][col + FS_2]
                                            - W[row + FS_2 - 1][col + FS_2 + 1])
                                            + W[row + FS_2 - 1][col + FS_2 + 1]));
                    }
                }
                else if(col == FS_2)
                {
                    s.x += (1 - fc.y) * (v1[FS_2].w * (fc.x * (W[row + FS_2][col + FS_2 - 1]
                                         - W[row + FS_2][col + FS_2]) + W[row + FS_2][col + FS_2])
                                         + v1[FS_2].z * fc.x * W[row + FS_2][col + FS_2]);
                    s.y += fc.y * (v1[FS_2].x * (fc.x * (W[row + FS_2][col + FS_2 - 1]
                                         - W[row + FS_2][col + FS_2] ) + W[row + FS_2][col + FS_2])
                                         + v1[FS_2].y * fc.x * W[row + FS_2][col + FS_2]);
                    if(row > -FS_2) {
                        s.z += (1 - fc.y) * (v0[FS_2].x * (fc.x * (W[row + FS_2 - 1][col + FS_2 - 1]
                                            - W[row + FS_2 - 1][col + FS_2])
                                            + W[row + FS_2 - 1][col + FS_2])
                                            + v0[FS_2].y * fc.x * W[row + FS_2 - 1][col + FS_2]);
                        s.w += fc.y * (v1[FS_2].w * (fc.x * (W[row + FS_2 - 1][col + FS_2 - 1]
                                            - W[row + FS_2 - 1][col + FS_2])
                                            + W[row + FS_2 - 1][col + FS_2])
                                            + v1[FS_2].z * fc.x * W[row + FS_2 - 1][col + FS_2]);
                    }
                }
                else
                {
                    s.x += (1 - fc.y) * (v1[(col + FS_2) / 2].w * (fc.x * (W[row + FS_2][col + FS_2 - 1]
                                        - W[row + FS_2][col + FS_2 + 0] ) + W[row + FS_2][col + FS_2 + 0])
                                        + v1[(col + FS_2) / 2].z * (fc.x * (W[row + FS_2][col + FS_2 - 0]
                                        - W[row + FS_2][col + FS_2 + 1]) + W[row + FS_2][col + FS_2 + 1]));
                    s.y += fc.y * (v1[(col + FS_2) / 2].x * (fc.x * (W[row + FS_2][col + FS_2-1]
                                        - W[row + FS_2][col + FS_2 + 0]) + W[row + FS_2][col + FS_2 + 0])
                                        + v1[(col + FS_2) / 2].y * (fc.x * (W[row + FS_2][col + FS_2 - 0]
                                        - W[row + FS_2][col + FS_2 + 1]) + W[row + FS_2][col + FS_2 + 1]));
                    if(row > -FS_2) {
                        s.z += (1 - fc.y) * (v0[(col + FS_2) / 2].x * (fc.x * (W[row + FS_2 - 1][col + FS_2 - 1]
                                                - W[row + FS_2 - 1][col + FS_2 + 0]) + W[row + FS_2 - 1][col + FS_2 + 0])
                                                + v0[(col + FS_2) / 2].y * (fc.x * (W[row + FS_2 - 1][col + FS_2 - 0]
                                                - W[row + FS_2 - 1][col + FS_2 + 1]) + W[row + FS_2 - 1][col + FS_2 + 1]));
                        s.w += fc.y * (v1[(col + FS_2) / 2].w * (fc.x * (W[row + FS_2 - 1][col + FS_2 - 1]
                                                - W[row + FS_2 - 1][col + FS_2 + 0]) + W[row + FS_2 - 1][col + FS_2 + 0])
                                                + v1[(col + FS_2) / 2].z * (fc.x * (W[row + FS_2 - 1][col + FS_2 - 0]
                                                - W[row + FS_2 - 1][col + FS_2 + 1]) + W[row + FS_2 - 1][col + FS_2 + 1]));
                    }
                }

                if(row != FS_2)
                    v0[(col + FS_2) / 2] = v1[(col + FS_2) / 2].xy;
            }
        }

        return dot(s, 1.0f) / w;
    #endif
}


float GetShadow( ShadowTex Input, float4 ShadowPosH )
{
    // Complete projection by doing division by w.
    float3 ShadowPos = ShadowPosH.xyz / ShadowPosH.w;

    float3 shadowPosDX = ddx_fine(ShadowPos);
    float3 shadowPosDY = ddy_fine(ShadowPos);

    float2 TransTexCoord = ShadowPos.xy;
    float Result = 1.f;
    if (!any(saturate(TransTexCoord) != TransTexCoord)) {

#if ShadowMode_ == ShadowModeSingle_
        Result = SampleSingle( Input, ShadowPos );
#elif ShadowMode_ == ShadowModeWeighted_
        Result = SampleWeighted( Input, ShadowPos );
#elif ShadowMode_ == ShadowModePoisson_
        Result = SamplePoissonDisk( Input, ShadowPos );
#elif ShadowMode_ == ShadowModePoissonRotated_
        Result = SamplePoissonDiskRotated( Input, ShadowPos );
#elif ShadowMode_ == ShadowModeOptimizedGaussinPCF_
        Result = SampletexShadowOptimizedGaussinPCF( Input, ShadowPos, shadowPosDX, shadowPosDY, 0 ); 
#elif ShadowMode_ == ShadowModePoissonStratified_
        Result = ShadowModePoissonDiskStratified( Input, ShadowPos );
#elif ShadowMode_ == ShadowModeFixedSizePCF_
        Result = SampleShadowMapFixedSizePCF( Input, ShadowPos, shadowPosDX, shadowPosDY, 0 );
#elif ShadowMode_ == ShadowModeGridPCF_
        Result = SampleShadowMapGridPCF( shadowPosition, shadowPosDX, shadowPosDY, cascadeIdx );
#endif
    }
    return Result * Result * 0.5 + 0.5;
}

