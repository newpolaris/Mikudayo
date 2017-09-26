#include "CommonInclude.hlsli"

SamplerState linearRepeat : register(s0);
SamplerState linearClamp : register(s1);

// Per-pixel color data passed through the pixel shader.
#if 0
struct PixelShaderInput
{
	float4 posHS : SV_POSITION;
	float3 posVS : POSITION0;
	float3 normalVS : NORMAL;
	float2 uv : TEXTURE;
};
#else
struct PixelShaderInput
{
	float4 posHS : SV_Position;
	float2 uv : TexCoords0;
};
#endif

cbuffer LightIndexBuffer : register(b4)
{
    // The index of the light in the Lights array.
    uint LightIndex;
}

Texture2D AmbientTextureVS : register(t0);
// The diffuse color from the view space texture.
Texture2D DiffuseTextureVS : register(t1);
// The specular color from the screen space texture.
Texture2D SpecularTextureVS : register(t2);
// The normal from the screen space texture.
Texture2D NormalTextureVS : register(t3);
// The depth from the screen space texture.
// Texture2D DepthTextureVS : register(t3);
Texture2D PositionTextureVS : register(t4);

// Deferred lighting pixel shader.
#if 1
// [earlydepthstencil]
float4 main( PixelShaderInput input ) : SV_Target
{
    // Everything is in view space.
    float4 eyePos = { 0, 0, 0, 1 };

    int2 texCoord = input.posHS.xy;
    // float depth = DepthTextureVS.Load( int3( texCoord, 0 ) ).r;

    // float4 P = ScreenToView( float4( texCoord, depth, 1.0f ) );
    float4 P = PositionTextureVS.Load( int3(texCoord, 0) );

    // View vector
    float4 V = normalize( eyePos - P );

    float4 ambient = AmbientTextureVS.Load( int3( texCoord, 0 ) );
    float4 diffuse = DiffuseTextureVS.Load( int3( texCoord, 0 ) );
    float4 specular = SpecularTextureVS.Load( int3( texCoord, 0 ) );
    float4 N = NormalTextureVS.Load( int3( texCoord, 0 ) );

    // Unpack the specular power from the alpha component of the specular color.
    float specularPower = exp2( specular.a * 10.5f );

    Material mat = (Material)0;
    mat.diffuse = diffuse.xyz;
    mat.alpha = diffuse.w;
    mat.specular = specular.xyz;
    mat.specularPower = specularPower;

    LightingResult result = DoLighting(Lights, mat, P.xyz, N.xyz);
    return diffuse * (result.Diffuse + ambient) + specular * result.Specular;
}
#else
// Pixel shader to render a texture to the screen.
float4 main( PixelShaderInput input ) : SV_Target
{
    // return float4( input.uv, 0, 1 );
    return SpecularTextureVS.SampleLevel( linearRepeat, input.uv, 0 );
    // return SpecularTextureVS.SampleLevel( linearRepeat, input.uv, 0 );
}
#endif
