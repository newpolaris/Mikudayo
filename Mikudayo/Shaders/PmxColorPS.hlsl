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
}

cbuffer MaterialConstants : register(b3)
{
    Material material;
    int sphereOperation;
    int bUseTexture;
    int bUseToon;
};

Texture2D<float4> texDiffuse : register(t1);
Texture2D<float3> texSphere : register(t2);
Texture2D<float3> texToon : register(t3);
Texture2D<float> ShadowTexture : register(t7);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);
SamplerComparisonState shadowSampler : register(s2);

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

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 lightVecVS = normalize(-SunDirectionVS);
    float3 normalVS = normalize(input.normalVS);
    float intensity = dot(lightVecVS, normalVS) * 0.5 + 0.5;
    float2 toonCoord = float2(0.5, 1.0 - intensity);

    float3 toEyeV = -input.posVS;
    float3 halfV = normalize(toEyeV + lightVecVS);

    float NdotH = dot(normalVS, halfV);
    float specularFactor = pow(max(NdotH, 0.001f), material.specularPower);
    float3 sunDiffuse = SunColor * max(0, dot(lightVecVS, normalVS));
    float3 sunSpecular = SunColor * specularFactor;

    float3 diffuse = material.diffuse;
    float3 ambient = material.ambient;
    float3 specular = material.specular;

    float texAlpha = 1.0;
    float3 texColor = float3(1.0, 1.0, 1.0);
    if (bUseTexture)
    {
        float4 tex = texDiffuse.Sample(sampler0, input.uv);
        texColor = tex.xyz;
        texAlpha = tex.w;
    }
    float2 sphereCoord = 0.5 + 0.5*float2(1.0, -1.0) * normalVS.xy;
    if (sphereOperation == kSphereAdd)
        texColor += texSphere.Sample(sampler0, sphereCoord);
    else if (sphereOperation == kSphereMul)
        texColor *= texSphere.Sample(sampler0, sphereCoord);
    if (bUseToon)
        texColor *= texToon.Sample(sampler0, toonCoord);

    diffuse = texColor * diffuse;
    ambient = texColor * ambient;

    LightingResult lit = DoLighting(Lights, material.specularPower, input.posVS, normalVS);

    float3 color = diffuse * lit.Diffuse.xyz + ambient + specular * lit.Specular.xyz;
    // float3 color = diffuse * lit.Diffuse.xyz + ambient * 0.1 + specular * lit.Specular.xyz;
    float alpha = texAlpha * material.alpha;

    Light light = (Light)0;
    light.DirectionVS = float4(SunDirectionVS, 1);
    light.Color = float4(SunColor, 1);
    LightingResult sunLit = DoDirectionalLight( light, material.specularPower, -input.posVS, normalVS );
    float shadow = GetShadow(input.shadowCoord);
    // color += shadow * (diffuse * sunLit.Diffuse + specular * sunLit.Specular);

    return float4(color, alpha);
}
