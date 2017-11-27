//
// Use code [NCHL]
//
#include "NchlColorVS.hlsli"
#include "Shadow.hlsli"
#define Toon 3

struct PixelShaderInput
{
    float4 positionCS : SV_POSITION;
    float3 positionWS : POSITION0;
    float3 eyeWS : POSITION1;
    float4 shadowPositionCS : POSITION2;
    float2 texCoord : TEXCOORD0;
    float2 spTex : TEXCOORD1;
    float3 normalWS : NORMAL;
};

struct PixelShaderOutput
{
    float4 color : SV_Target0;   // color pixel output (R11G11B10_FLOAT)
    float4 emissive : SV_Target1; // emissive color output (R11G11B10_FLOAT)
};

Texture2D<float4> texDiffuse : register(t1);
Texture2D<float4> texSphere : register(t2);
Texture2D<float4> texToon : register(t3);

Texture2D<float> texSSAO : register(t64);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

// Beckmanormal distribution
float Beckmann(float m, float nh)
{
    float nh2 = nh*nh;
    float m2 = m*m;
    return exp((nh2-1)/(nh2*m2)) / (PI*m2*nh2*nh2);
}

float Pow5(float n)
{
    return n*n*n*n*n;
}

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output;
    Material mat = Mat;

    float3 normal = input.normalWS;
    if (any( normal ))
        normal = normalize( normal );

    float halfLambertTerm = max(dot( normal, -LightDirection ), 0)*0.5 + 0.5;
    float4 color = DiffuseColor;
    // color = halfLambertTerm * color;

    uint2 pixelPos = input.positionCS.xy;

    if (mat.bUseTexture) {
        float4 texColor = texDiffuse.Sample( sampler0, input.texCoord );
        color *= texColor;
        // shadowColor *= texColor;
    }

#if 0
    const bool bUseNormalTexture = mat.sphereOperation != kSphereNone;
    if (bUseNormalTexture) {
        float4 texColor = texSphere.Sample( sampler0, input.spTex );
        if (mat.sphereOperation == kSphereAdd) {
            color.rgb += texColor.rgb;
            // shadowColor.rgb += texColor.rgb;
        } else {
            color.rgb *= texColor.rgb;
            // shadowColor.rgb *= texColor.rgb;
        }
        color.a *= texColor.a;
        shadowColor.a *= texColor.a;
    }
#endif

    // Complete projection by doing division by w.
    float3 shadowCoord = input.shadowPositionCS.xyz / input.shadowPositionCS.w;
    float comp = 1.0;
    if (!any(saturate(shadowCoord.xy) != shadowCoord.xy))
    {
        shadowCoord = saturate( shadowCoord );
        comp = GetShadow( input.shadowPositionCS, input.positionCS.xyz );
        if (mat.bUseToon) {
            float lightIntensity = dot( normal, -SunDirectionWS );
            comp = min( saturate( lightIntensity )*Toon, comp );
            // shadowColor *= MaterialToon;
        }
    }
    comp = min(comp, texSSAO[pixelPos]);
    // color = lerp( shadowColor, color, comp );
    output.color = color;
    output.emissive = color;
    return output;
}