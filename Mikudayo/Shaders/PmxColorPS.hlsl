#include "CommonInclude.hlsli"

//
// Use code full.fx, AutoLuminous4.fx
//

struct PixelShaderInput
{
    float4 positionHS : SV_POSITION;
    float4 positionCS : POSITION1;
    float3 positionWS : POSITION2;
    float3 eyeWS : POSITION0;
    float2 texCoord : TEXCOORD0;
    float2 spTex : TEXCOORD1;
    float3 normalWS : NORMAL;
    float4 color : COLOR0;
    float3 specular : COLOR1;
    float4 emissive : COLOR2;
};

struct PixelShaderOutput
{
    float4 color : SV_Target0;   // color pixel output (R11G11B10_FLOAT)
    float4 emissive : SV_Target1; // emissive color output (R11G11B10_FLOAT)
};

struct Material
{
	float3 diffuse;
	float alpha;
	float3 specular;
	float specularPower;
	float3 ambient;
    int sphereOperation;
    int bUseTexture;
    int bUseToon;
    float EdgeSize;
    float4 EdgeColor;
};

cbuffer Constants: register(b0)
{
	matrix view;
	matrix projection;
    matrix viewToShadow;
    float3 cameraPositionWS;
};

cbuffer MaterialConstants : register(b4)
{
    Material Mat;
};

static const int kSphereNone = 0;
static const int kSphereMul = 1;
static const int kSphereAdd = 2;

cbuffer PSConstants : register(b5)
{
    float3 SunDirectionWS;
    float3 SunColor;
    float4 ShadowTexelSize;
}

cbuffer ReflectorPlane : register(b11)
{
    float4 reflectPlane;
}

Texture2D<float4> texDiffuse : register(t1);
Texture2D<float4> texSphere : register(t2);
Texture2D<float4> texToon : register(t3);
Texture2D<float> ShadowTexture : register(t7);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);
SamplerComparisonState shadowSampler : register(s2);

float GetShadow( float3 ShadowCoord )
{
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

float DistanceFromReflector( float3 position )
{
    return dot(reflectPlane.xyz, position.xyz) + reflectPlane.a;
}

PixelShaderOutput main(PixelShaderInput input)
{ 
    PixelShaderOutput output;
    Material mat = Mat;

    clip(-DistanceFromReflector( input.positionWS ));

    float4 color = input.color;
    float4 emissive = input.emissive;

    if (mat.bUseTexture)
        color *= texDiffuse.Sample( sampler0, input.texCoord );

    if (mat.sphereOperation != kSphereNone) {
        float4 texColor = texSphere.Sample( sampler0, input.spTex );
        if (mat.sphereOperation == kSphereAdd)
            color.rgb += texColor.rgb;
        else
            color.rgb *= texColor.rgb;
        color.a *= texColor.a;
    }

    if (mat.bUseToon) {
        float lightNormal = dot( input.normalWS, -SunDirectionWS );
        color *= texToon.Sample( sampler1, float2(0, 0.5 - lightNormal * 0.5) );
    }

    color.rgb += input.specular;
    output.color = color;
    output.emissive = color * emissive;
    return output;
}
