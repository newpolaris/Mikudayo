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

// The diffuse color from the view space texture.
Texture2D<float4> DiffuseTextureVS : register(t4);
// The specular color from the screen space texture.
Texture2D<float4> SpecularTextureVS : register(t5);
Texture2D<float3> NormalTextureVS : register(t6);

[earlydepthstencil]
float4 main( PixelShaderInput input ) : SV_Target
{
    int2 texCoord = input.posHS.xy;
    float3 lightVecVS = normalize( -SunDirectionVS );

    LightingResult lit;
    lit.Diffuse = DiffuseTextureVS.Load( int3(texCoord, 0) );
    lit.Specular = SpecularTextureVS.Load( int3(texCoord, 0) );

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

    // Ambient = float4(texColor * ambient, 1.0);
    float4 AmbientColor = float4(texColor * ambient * 0.1, 1.0);
    float4 DiffuseColor = float4(texColor * diffuse, 1.0);
    float4 SpecularColor = float4(specular, 1.0);

    return (AmbientColor + DiffuseColor) * lit.Diffuse + SpecularColor * lit.Specular;
}

