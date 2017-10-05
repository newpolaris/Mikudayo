#include "CommonInclude.hlsli"

struct PixelShaderOutput
{
    float4 Diffuse              : SV_Target0;   // Diffuse Albedo (R16G16B16_FLOAT) Unused (A8_UNORM)
    float4 Specular             : SV_Target1;   // Specular Color (R16G16B16_FLOAT) Unused (A8_UNORM)
};

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

// The normal from the screen space texture.
Texture2D<float3> NormalTextureVS : register(t0);
// The specular power from the screen space texture.
Texture2D<float> SpecularPowerTextureVS : register(t1);
// The depth from the screen space texture.
Texture2D<float> DepthTextureVS : register(t2);

// Deferred lighting pixel shader.
[earlydepthstencil]
PixelShaderOutput main( float4 PosHS : SV_Position )
{
    PixelShaderOutput Out;

    int2 texCoord = PosHS.xy;
    float depth = DepthTextureVS.Load( int3( texCoord, 0 ) );
    float3 P = ScreenToView( float4( texCoord, depth, 1.0f ) ).xyz;

    // View vector
    float3 V = normalize(-P);
    // Unpack the normal
    float3 N = NormalTextureVS.Load( int3( texCoord, 0 ) ) * 2 - 1;
    // Unpack the specular power
    float specularPower = SpecularPowerTextureVS.Load( int3( texCoord, 0 ) ) * 255.0;

    Light light = Lights[LightIndex];
    LightingResult lit = (LightingResult)0;
    switch ( light.Type )
    {
    case DirectionalLight:
        lit = DoDirectionalLight( light, specularPower, V, N );
        break;
    case PointLight:
        lit = DoPointLight( light, specularPower, V, P, N );
        break;
    case SpotLight:
        lit = DoSpotLight( light, specularPower, V, P, N );
        break;
    }

    Out.Diffuse = lit.Diffuse;
    Out.Specular = lit.Specular;

    return Out;
}
