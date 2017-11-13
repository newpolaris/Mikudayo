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
    float3 normal = input.normal;

    // Transform the vertex position into projected space.
    matrix worldViewProjMatrix = mul( projection, mul( view, model ) );
    output.positionWS = mul( (float3x3)model, position );
    output.positionHS = mul( worldViewProjMatrix, float4(position, 1) );
    output.shadowPositionCS = mul( viewToShadow, float4(output.positionWS, 1) );
    output.eyeWS = cameraPosition - mul( (float3x3)model, position );
    output.normalWS = normalize(mul( (float3x3)model, normal ));
    output.color.rgb = AmbientColor;
    if (!mat.bUseToon) {
        output.color.rgb += max( 0, dot( output.normalWS, -SunDirectionWS ) ) * DiffuseColor.rgb;
    }
    output.color.a = DiffuseColor.a;
    output.color = saturate( output.color );
	output.texCoord = input.texcoord;
    if ( mat.sphereOperation != kSphereNone ) {
        float2 normalVS = mul( (float3x3)view, output.normalWS ).xy;
        output.spTex.x = normalVS.x * 0.5 + 0.5;
        output.spTex.y = normalVS.y * -0.5 + 0.5;
    }
    float3 halfVector = normalize( normalize( output.eyeWS ) + -SunDirectionWS );
    output.specular = pow( max( 0, dot( halfVector, output.normalWS ) ), mat.specularPower ) * SpecularColor;
    output.emissive = float4(0, 0, 0, MaterialDiffuse.a);
#if AUTOLUMINOUS
    if (IsEmission)
    {
        output.emissive = MaterialDiffuse;
        // from Autoluminous 'EmittionPower0'
        float factor = max( 1, (mat.specularPower - 100) / 7 );
        output.emissive.rgb *= factor*10;
        output.color.rgb += lerp(float3(1, 1, 1), output.color.rgb, 0.0) * factor;
    }
#endif
    output.ambient = AmbientColor;
	return output;
}
