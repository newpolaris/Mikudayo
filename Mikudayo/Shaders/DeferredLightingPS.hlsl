#include "CommonInclude.hlsli"

SamplerState linearRepeat : register(s0);
SamplerState linearClamp : register(s1);

cbuffer PassConstants : register(b1)
{
    float3 SunDirectionVS;
    float3 SunColor;
}

cbuffer LightIndexBuffer : register(b4)
{
    // The index of the light in the Lights array.
    uint LightIndex;
}

// The diffuse color from the view space texture.
Texture2D DiffuseTextureVS : register(t0);
// The specular color from the screen space texture.
Texture2D SpecularTextureVS : register(t1);
// The normal from the screen space texture.
Texture2D NormalTextureVS : register(t2);
// The depth from the screen space texture.
// Texture2D DepthTextureVS : register(t3);
Texture2D PositionTextureVS : register(t3);

// Deferred lighting pixel shader.
[earlydepthstencil]
float4 main( float4 posHS : SV_Position ) : SV_Target
{
    int2 texCoord = posHS.xy;
    // float depth = DepthTextureVS.Load( int3( texCoord, 0 ) ).r;

    // float4 P = ScreenToView( float4( texCoord, depth, 1.0f ) );
    float3 P = PositionTextureVS.Load( int3(texCoord, 0) ).xyz;

    // View vector
    // float3 V = normalize(-P);
    float3 V = -P;

    float4 diffuse = DiffuseTextureVS.Load( int3( texCoord, 0 ) );
    float4 specular = SpecularTextureVS.Load( int3( texCoord, 0 ) );
    float3 N = NormalTextureVS.Load( int3( texCoord, 0 ) ).xyz;

    // Unpack the specular power from the alpha component of the specular color.
    float specularPower = exp2( specular.a * 10.5f );

    Material mat = (Material)0;
    mat.diffuse = diffuse.xyz;
    mat.alpha = diffuse.w;
    mat.specular = specular.xyz;
    mat.specularPower = specularPower;

    Light light = Lights[LightIndex];
    LightingResult lit = (LightingResult)0;
    switch ( light.Type )
    {
    case DirectionalLight:
        lit = DoDirectionalLight( light, mat, V, N );
        break;
    case PointLight:
        lit = DoPointLight( light, mat, V, P, N );
        break;
    case SpotLight:
        lit = DoSpotLight( light, mat, V, P, N );
        break;
    }
    return diffuse * lit.Diffuse + specular * lit.Specular;
}
