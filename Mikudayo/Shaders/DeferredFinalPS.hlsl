#include "CommonInclude.hlsli"

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
    float4 posHS : SV_POSITION;
    float3 posVS : POSITION0;
    float3 normalVS : NORMAL;
    float2 uv : TEXTURE0;
    float3 shadowCoord : TEXTURE1;
};

static const int kSphereNone = 0;
static const int kSphereMul = 1;
static const int kSphereAdd = 2;

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
SamplerComparisonState shadowSampler : register(s2);

// The diffuse color from the view space texture.
Texture2D<float3> DiffuseTextureVS : register(t4);
// The specular color from the screen space texture.
Texture2D<float3> SpecularTextureVS : register(t5);
Texture2D<float3> NormalTextureVS : register(t6);
Texture2D<float> ShadowTexture : register(t7);

float GetShadow( float3 ShadowCoord )
{
#define SINGLE_SAMPLE
#ifdef SINGLE_SAMPLE
    float result = ShadowTexture.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy, ShadowCoord.z );
#else
    const float Dilation = 2.0;
    float d1 = Dilation * ShadowTexelSize.x * 0.125;
    float d2 = Dilation * ShadowTexelSize.x * 0.875;
    float d3 = Dilation * ShadowTexelSize.x * 0.625;
    float d4 = Dilation * ShadowTexelSize.x * 0.375;
    float result = (
        2.0 * ShadowTexture.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy, ShadowCoord.z ) +
        ShadowTexture.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2(-d2,  d1), ShadowCoord.z ) +
        ShadowTexture.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2(-d1, -d2), ShadowCoord.z ) +
        ShadowTexture.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2( d2, -d1), ShadowCoord.z ) +
        ShadowTexture.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2( d1,  d2), ShadowCoord.z ) +
        ShadowTexture.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2(-d4,  d3), ShadowCoord.z ) +
        ShadowTexture.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2(-d3, -d4), ShadowCoord.z ) +
        ShadowTexture.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2( d4, -d3), ShadowCoord.z ) +
        ShadowTexture.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2( d3,  d4), ShadowCoord.z )
        ) / 10.0;
#endif
    return result * result;
}

[earlydepthstencil]
float4 main( PixelShaderInput input ) : SV_Target
{
    int2 texCoord = input.posHS.xy;
    float3 lightVecVS = normalize( -SunDirectionVS );

    LightingResult lit;
    lit.Diffuse = float4(DiffuseTextureVS.Load( int3(texCoord, 0) ), 1);
    lit.Specular = float4(SpecularTextureVS.Load( int3(texCoord, 0) ), 1);

    float3 normalVS = NormalTextureVS.Load( int3(texCoord, 0) ) * 2 - 1;
    float intensity = dot( lightVecVS, normalVS ) * 0.5 + 0.5;
    float2 toonCoord = float2(0.5, 1.0 - intensity);

    float3 diffuse = material.diffuse;
    float3 ambient = material.ambient;
    float3 specular = material.specular;

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

    // float4 AmbientColor = float4(texColor * ambient * 1.0, 1.0);
    float4 AmbientColor = float4(texColor * ambient * 0.1, 1.0);
    float4 DiffuseColor = float4(texColor * diffuse, 1.0);
    float4 SpecularColor = float4(specular, 1.0);

    float4 colorSum = AmbientColor + DiffuseColor * lit.Diffuse + SpecularColor * lit.Specular;

    float3 N = normalize( input.normalVS );

    Light light = (Light)0;
    light.DirectionVS = float4(SunDirectionVS, 1);
    light.Color = float4(SunColor, 1);
    LightingResult sunLit = DoDirectionalLight( light, material.specularPower, -input.posVS, N );
    float shadow = GetShadow(input.shadowCoord);
    colorSum += shadow * (DiffuseColor * sunLit.Diffuse + SpecularColor * sunLit.Specular);

    return colorSum;
}

