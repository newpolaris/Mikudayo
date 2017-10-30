#include "CommonInclude.hlsli"
#include "MultiLight.hlsli"

#define SKII1 1500
#define Toon 3

//
// Use code full.fx, AutoLuminous4.fx
//

struct PixelShaderInput
{
    float4 positionHS : SV_POSITION;
    float3 positionWS : POSITION0;
    float3 eyeWS : POSITION1;
    float4 shadowPositionCS : POSITION2;
    float2 texCoord : TEXCOORD0;
    float2 spTex : TEXCOORD1;
    float3 normalWS : NORMAL;
    float4 color : COLOR0;
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
    float4 MaterialToon;
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

Texture2D<float> texShadow : register(t62);
Texture2D<float4> texReflectDiffuse : register(t63);
Texture2D<float4> texReflectEmmisive : register(t64);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);
SamplerComparisonState shadowSampler : register(s2);

float GetShadow( float3 ShadowCoord )
{
#define SINGLE_SAMPLE
#ifdef SINGLE_SAMPLE
    float result = texShadow.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy, ShadowCoord.z );
#else
    const float Dilation = 2.0;
    float d1 = Dilation * ShadowTexelSize.x * 0.125;
    float d2 = Dilation * ShadowTexelSize.x * 0.875;
    float d3 = Dilation * ShadowTexelSize.x * 0.625;
    float d4 = Dilation * ShadowTexelSize.x * 0.375;
    float result = (
        2.0 * texShadow.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy, ShadowCoord.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2(-d2,  d1), ShadowCoord.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2(-d1, -d2), ShadowCoord.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2( d2, -d1), ShadowCoord.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2( d1,  d2), ShadowCoord.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2(-d4,  d3), ShadowCoord.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2(-d3, -d4), ShadowCoord.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2( d4, -d3), ShadowCoord.z ) +
        texShadow.SampleCmpLevelZero( shadowSampler, ShadowCoord.xy + float2( d3,  d4), ShadowCoord.z )
        ) / 10.0;
#endif
    return result * result;
}

float DistanceFromReflector( float3 position )
{
    return dot(reflectPlane.xyz, position.xyz) + reflectPlane.a;
}

static float3 LightSpecular = SunColor;
static float3 LightDirection = SunDirectionWS;
static float3 MaterialSpecular = Mat.specular;
static float3 SpecularColor = MaterialSpecular * LightSpecular;
static float SpecularPower = Mat.specularPower;
static const float4 MaterialToon = Mat.MaterialToon;

#if !REFLECTED
[earlydepthstencil]
#endif
PixelShaderOutput main(PixelShaderInput input)
{ 
    PixelShaderOutput output;
    Material mat = Mat;

#if REFLECTED
    clip(-DistanceFromReflector( input.positionWS ));
#endif

    float3 normal = normalize( input.normalWS );
    float3 view = normalize(input.eyeWS);
    float3 HalfVector = normalize( view + -LightDirection );
    float3 Specular = pow( max( 0, dot( HalfVector, normal ) ), SpecularPower + SP_Power ) * (SpecularColor + SP_Add) * SP_Scale * MainLightParam;

    for (int i = 0; i < 16; i++)
    {
        HalfVector = normalize( view + -normalize( LightPos[i] ) );
        Specular += 0.16*pow( max( 0, dot( HalfVector, normal ) ), SpecularPower + SP_Power ) * (SpecularColor + SP_Add) * SP_Scale * SubLightParam;
    }

    float4 color = input.color;
    float4 shadowColor = input.color;
    float4 emissive = input.emissive;

    if (mat.bUseTexture) {
        float4 texColor = texDiffuse.Sample( sampler0, input.texCoord );
        color *= texColor;
        shadowColor *= texColor;
    }

    if (mat.sphereOperation != kSphereNone) {
        float4 texColor = texSphere.Sample( sampler0, input.spTex );
        if (mat.sphereOperation == kSphereAdd) {
            color.rgb += texColor.rgb;
            shadowColor.rgb += texColor.rgb;
        } else {
            color.rgb *= texColor.rgb;
            shadowColor.rgb *= texColor.rgb;
        }
        color.a *= texColor.a;
        shadowColor.a *= texColor.a;
    }

    float3 Rim = pow( 1 - saturate(max(0, dot(normal, view))), RimPow ) * RimLight * pow( color.rgb, 2 );
    color.rgb += Rim;
    shadowColor.rgb += Rim;
    color.rgb += Specular;
    shadowColor.rgb += Specular*0.5;
    
    color.rgb *= BaseBias;
    shadowColor.rgb *= ShadowBias;
    
    color.a = saturate(color.a);
    shadowColor.a = saturate(shadowColor.a);

#if REFLECTOR
    float4 texMirrorColor = texReflectDiffuse.Load( int3(input.positionHS.xy, 0) );
    color *= texMirrorColor;
    shadowColor *= texMirrorColor;
    float4 texMirrorEmmisive = texReflectEmmisive.Load( int3(input.positionHS.xy, 0) );
    emissive *= texMirrorEmmisive;
#endif

    // Complete projection by doing division by w.
    float3 shadowPositionNS = input.shadowPositionCS.xyz / input.shadowPositionCS.w;
    float2 shadowCoord = shadowPositionNS.xy * float2(0.5, -0.5) + 0.5;

#if 0
    if (any(shadowCoord == saturate(shadowCoord)))
    {
        float comp = GetShadow( shadowCoord );
        color = float4(comp.xxx, 1);
    }
#else 
    if (any(saturate(shadowCoord) == shadowCoord))
    {
        float comp = saturate(max(- shadowPositionNS.z + texShadow.Sample(sampler1, shadowCoord), 0.0f)*SKII1 - 0.3f);
        if (mat.bUseToon) {
            comp = min(saturate(dot(input.normalWS,-LightDirection)*Toon),comp);
            shadowColor *= MaterialToon;
        }
        color = lerp(shadowColor, color, 1);
    }
#endif
    output.color = color;
    output.emissive = color * emissive;
    return output;
}
