#include "CommonInclude.hlsli"
#include "MikuColorVS.hlsli"

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

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(VertexShaderInput input)
{
    PixelShaderInput output = (PixelShaderInput)0;
    Material mat = Mat;

    float3 position = input.position;

    // Transform the vertex position into projected space.
    matrix worldViewProjMatrix = mul( projection, mul( view, model ) );
    output.positionWS = mul( (float3x3)model, position );
    output.positionHS = mul( worldViewProjMatrix, float4(position, 1) );
    output.shadowPositionCS = mul( viewToShadow, float4(output.positionWS, 1) );
    output.eyeWS = cameraPosition - mul( (float3x3)model, position );
    float3 normalWS = mul( (float3x3)model, input.normal );
    if (any( normalWS ))
        normalWS = normalize( normalWS );
    output.normalWS = normalWS;
    output.color.rgb = AmbientColor;
    if (!mat.bUseToon) {
        output.color.rgb += max( 0, dot( normalWS, -SunDirectionWS ) ) * DiffuseColor.rgb;
    }
    output.color.a = DiffuseColor.a;
    output.color = saturate( output.color );
	output.texCoord = input.texcoord;
    if ( mat.sphereOperation != kSphereNone ) {
        float2 normalVS = mul( (float3x3)view, normalWS ).xy;
        output.spTex = normalVS * float2(0.5, -0.5) + float2(0.5, 0.5);
    }
    float3 halfVector = normalize( normalize( output.eyeWS ) + -SunDirectionWS );
    float specular = max( 0, dot( halfVector, output.normalWS ) );
    if (any(specular))
        output.specular = pow( specular, mat.specularPower ) * SpecularColor;
    output.emissive = float4(0, 0, 0, MaterialDiffuse.a);
#if AUTOLUMINOUS
    if (IsEmission)
    {
        output.emissive = MaterialDiffuse;
        // from Autoluminous 'EmittionPower0'
        float factor = max( 1, (mat.specularPower - 100) / 7 );
        output.emissive.rgb *= factor*2;
        output.color.rgb *= factor*2;
    }
#endif
    output.ambient = AmbientColor;
	return output;
}
