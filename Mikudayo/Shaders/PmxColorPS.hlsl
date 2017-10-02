#include "CommonInclude.hlsli"

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
    float4 posHS : SV_POSITION;
    float3 posVS : POSITION0;
    float3 normalVS : NORMAL;
    float2 uv : TEXTURE;
};

static const int kSphereNone = 0;
static const int kSphereMul = 1;
static const int kSphereAdd = 2;

cbuffer MaterialConstants : register(b0)
{
    Material material;
    int sphereOperation;
    int bUseTexture;
    int bUseToon;
};

cbuffer PassConstants : register(b1)
{
    float3 SunDirectionVS;
    float3 SunColor;
}

Texture2D<float4> texDiffuse : register(t1);
Texture2D<float3> texSphere : register(t2);
Texture2D<float3> texToon : register(t3);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

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

    LightingResult lit = DoLighting(Lights, material, input.posVS, normalVS);
    float3 color = diffuse * lit.Diffuse.xyz + ambient + specular * lit.Specular.xyz;
    float alpha = texAlpha * material.alpha;
    return float4(color, alpha);
}
