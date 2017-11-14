#include "CommonInclude.hlsli"
#include "MikuColorVS.hlsli"
#include "MultiLight.hlsli"

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

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(VertexShaderInput input)
{
    static const float3 ambientColor = saturate(AmbientColor);

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

    output.color.rgb = max(0, dot(normalWS, -LightDirection) * MainHLamb + (1 - MainHLamb))*ambientColor*MainLightParam;
    float3 subsum = 0;
    for (int i = 0; i < 16; i++) {
        float3 p = float3(LightPos[i].xy, -LightPos[i].z);
        subsum += max(0, dot(normalWS, normalize(p)) * SubHLamb + (1 - SubHLamb))*ambientColor*SubLightParam;
    }
    
    output.color.rgb += (subsum / (16 * LightScale))*1.0;
    output.color.a = smoothstep( 0, 0.9, DiffuseColor.a );
	output.texCoord = input.texcoord;
    if ( mat.sphereOperation != kSphereNone ) {
        float2 normalVS = mul( (float3x3)view, normalWS ).xy;
        output.spTex = normalVS * float2(0.5, -0.5) + float2(0.5, 0.5);
    }
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
	return output;
}
