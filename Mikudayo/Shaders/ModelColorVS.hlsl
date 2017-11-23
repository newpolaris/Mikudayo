#include "ModelColorVS.hlsli"

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
    Material mat = Mat;

    // Transform the vertex position into projected space.
    matrix worldViewProjMatrix = mul( projection, mul( view, model ) );
    float4 position = float4(input.position, 1);
    output.positionWS  = mul( (float3x3)model, input.position );
    output.positionHS = mul( worldViewProjMatrix, position );
    output.positionCS = output.positionHS;
    output.eyeWS = cameraPosition - mul( (float3x3)model, input.position );
    output.normalWS = normalize(mul( (float3x3)model, input.normal ));
    output.color.rgb = AmbientColor;
    output.color.rgb += max( 0, dot( output.normalWS, -SunDirectionWS ) ) * DiffuseColor.rgb;
    output.color.a = DiffuseColor.a;
    output.emissive = float4(MaterialEmissive, MaterialDiffuse.a);
	output.texCoord = input.texcoord;
    float3 halfVector = normalize( normalize( output.eyeWS ) + -SunDirectionWS );
    float specular = max( 0.00001, dot( halfVector, output.normalWS ) );
    if (any(specular))
        output.specular = pow( specular, mat.shininess ) * SpecularColor;
#if AUTOLUMINOUS
    if (IsEmission)
    {
        output.emissive.rgb += MaterialDiffuse.rgb;
        // from Autoluminous 'EmittionPower0'
        float factor = max( 1, (mat.shininess - 100) / 7 );
        output.emissive.rgb *= factor*5;
        output.color.rgb *= factor*2;
    }
#endif
	return output;
}
