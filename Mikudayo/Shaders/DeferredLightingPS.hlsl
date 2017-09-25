#include "CommonInclude.hlsli"

SamplerState linearRepeat : register(s0);
SamplerState linearClamp : register(s1);

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 posHS : SV_POSITION;
	float3 posVS : POSITION0;
	float3 normalVS : NORMAL;
	float2 uv : TEXTURE;
};

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
Texture2D DepthTextureVS : register(t3);

// Deferred lighting pixel shader.
#if 1
[earlydepthstencil]
float4 main( PixelShaderInput input ) : SV_Target
{
    // Everything is in view space.
    float4 eyePos = { 0, 0, 0, 1 };

    int2 texCoord = input.posHS.xy;
    float depth = DepthTextureVS.Load( int3( texCoord, 0 ) ).r;

    float4 P = ScreenToView( float4( texCoord, depth, 1.0f ) );

    // View vector
    float4 V = normalize( eyePos - P );

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

    LightingResult result = (LightingResult)0;
    for (int i = 0; i < NumLights; i++)
    {
        LightingResult lit = (LightingResult)0;
        Light light = Lights[i];

        switch (light.Type)
        {
        case DirectionalLight:
            lit = DoDirectionalLight( light, mat, V.xyz, N.xyz );
            break;
        case PointLight:
            lit = DoPointLight( light, mat, V.xyz, P.xyz, N.xyz );
            break;
        case SpotLight:
            lit = DoSpotLight( light, mat, V.xyz, P.xyz, N.xyz );
            break;
        }
        result.Diffuse += lit.Diffuse;
        result.Specular += lit.Specular;
    }
    return ( diffuse * result.Diffuse ) + ( specular * result.Specular );
}

#else
// Pixel shader to render a texture to the screen.
float4 main( PixelShaderInput input ) : SV_Target
{
    return float4( input.uv, 0, 1 );
    //return DiffuseTextureVS.SampleLevel( linearRepeat, input.uv, 0 );
    //return DebugTexture.Sample( LinearRepeatSampler, input.texCoord );
}
#endif
