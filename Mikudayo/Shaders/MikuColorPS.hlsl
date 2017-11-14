#include "CommonInclude.hlsli"
#include "MikuColorVS.hlsli"
#include "Shadow.hlsli"
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
    float3 specular : COLOR1;
    float4 emissive : COLOR2;
    float3 ambient : COLOR3;
};

struct PixelShaderOutput
{
    float4 color : SV_Target0;   // color pixel output (R11G11B10_FLOAT)
    float4 emissive : SV_Target1; // emissive color output (R11G11B10_FLOAT)
};

cbuffer ReflectorPlane : register(b11)
{
    float4 reflectPlane;
}

Texture2D<float4> texDiffuse : register(t1);
Texture2D<float4> texSphere : register(t2);
Texture2D<float4> texToon : register(t3);

Texture2D<float4> texReflectDiffuse : register(t63);
Texture2D<float4> texReflectEmmisive : register(t64);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

float DistanceFromReflector( float3 position )
{
    return dot(reflectPlane.xyz, position.xyz) + reflectPlane.a;
}

static float SpecularPower = Mat.specularPower;

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

    float4 color = input.color;
    float4 shadowColor = float4(saturate(input.ambient), color.a);
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

    color.rgb += input.specular;

#if REFLECTOR
    float4 texMirrorColor = texReflectDiffuse.Load( int3(input.positionHS.xy, 0) );
    color *= texMirrorColor;
    shadowColor *= texMirrorColor;
    float4 texMirrorEmmisive = texReflectEmmisive.Load( int3(input.positionHS.xy, 0) );
    emissive *= texMirrorEmmisive;
#endif
    float3 normal = normalize( input.normalWS );
    // Complete projection by doing division by w.
    float3 shadowCoord = input.shadowPositionCS.xyz / input.shadowPositionCS.w;
    if (!any(saturate(shadowCoord.xy) != shadowCoord.xy))
    {
        shadowCoord = saturate( shadowCoord );
        float comp = GetShadow( input.shadowPositionCS, input.positionHS.xyz );
        if (mat.bUseToon) {
            float lightIntensity = dot( normal, -SunDirectionWS );
            comp = min( saturate( lightIntensity )*Toon, comp );
            shadowColor *= MaterialToon;
        }
        color = lerp( shadowColor, color, comp );
    }
    output.color = color;
    output.emissive = color * emissive;
    return output;
}
