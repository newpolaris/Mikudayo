#include "CommonInclude.hlsli"

#define AUTOLUMINOUS 1
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
    float3  diffuse;
    //-------------------------- ( 16 bytes )
    float3  specular;
    //-------------------------- ( 16 bytes )
    float3  ambient;
    //-------------------------- ( 16 bytes )
    float3  emissive;
    //-------------------------- ( 16 bytes )
    float3  transparent;
    float   padding0;
    //-------------------------- ( 16 bytes )
    float   opacity;
    float   shininess;
    float   specularStrength;
    bool    bDiffuseTexture;
    //-------------------------- ( 16 bytes )
    bool    bSpecularTexture;
    bool    bEmissiveTexture;
    bool    bNormalTexture;
    bool    bLightmapTexture;
    //-------------------------- ( 16 bytes )
    bool    bReflectionTexture;
    // float3  padding1;
    //--------------------------- ( 16 bytes )
};

cbuffer Material : register(b4)
{
    Material Mat;
};


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
Texture2D<float4> texDiffuse		: register(t1);
Texture2D<float3> texSpecular		: register(t2);
Texture2D<float4> texEmissive		: register(t3);
Texture2D<float3> texNormal			: register(t4);
Texture2D<float4> texLightmap		: register(t5);
Texture2D<float4> texReflection	    : register(t6);
Texture2D<float> ShadowTexture : register(t7);

Texture2D<float> texSSAO			: register(t64);
Texture2D<float> texShadow			: register(t65);

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

#if !REFLECTED
#endif
PixelShaderOutput main(PixelShaderInput input)
{ 
    PixelShaderOutput output;
    Material mat = Mat;

#if REFLECTED
    clip(-DistanceFromReflector( input.positionWS ));
#endif
    float4 color = input.color;
    float4 emissive = input.emissive;
    if (mat.bDiffuseTexture) {
        color *= texDiffuse.Sample( sampler0, input.texCoord );
        emissive *= texDiffuse.Sample( sampler0, input.texCoord );
    }
    color.rgb += input.specular;
    output.color = color;
    output.emissive = emissive;
    return output;
}