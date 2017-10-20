#include "CommonInclude.hlsli"

struct Material
{
    float3  diffuse;
    //-------------------------- ( 16 bytes )
    float3  specular;
    //-------------------------- ( 16 bytes )
    float3  ambient;
    //-------------------------- ( 16 bytes )
    float3  emissive;
    //-------------------------- ( 16 bytes )
    float4  transparent;
    //-------------------------- ( 16 bytes )
    float   opacity;
    float   shininess;
    float   specularStrength;
    bool    bDiffuseTexture;
    //-------------------------- ( 16 bytes )
    bool    bSpecularTexture;
    bool    bEmissiveTexture;
    bool    bNormalTexture;
    bool    bLightmapTexture;
    //-------------------------- ( 16 bytes )
    bool    bReflectionTexture;
    // float  padding;
    //--------------------------- ( 16 bytes )
};

cbuffer Material : register(b4)
{
    Material Mat;
};

struct PixelShaderInput
{
    float4 positionHS : SV_POSITION;
    float3 positionVS : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 shadowCoord : TEXCOORD1;
    float3 normalVS : NORMAL;
    float3 tangentVS : TANGENT;
    float3 bitangentVS : BITANGENT;
};

Texture2D<float4> texDiffuse		: register(t1);
Texture2D<float3> texSpecular		: register(t2);
Texture2D<float4> texEmissive		: register(t3);
Texture2D<float3> texNormal			: register(t4);
Texture2D<float4> texLightmap		: register(t5);
Texture2D<float4> texReflection	    : register(t6);

Texture2D<float> texSSAO			: register(t64);
Texture2D<float> texShadow			: register(t65);

SamplerState sampler0 : register(s0);
SamplerComparisonState shadowSampler : register(s1);

struct PixelShaderOutput
{
    float depth : SV_Depth;
    float color : SV_Target;
};

float4 main(PixelShaderInput input) : SV_TARGET
{ 
    Material mat = Mat;

    // float alpha = 1.0; // mat.opacity;
    float alpha = mat.opacity;
    float3 diffuse = mat.diffuse;
    if (mat.bDiffuseTexture)
    {
        float4 diffuseTex = texDiffuse.Sample( sampler0, input.texCoord );
        if (any(diffuse.rgb))
            diffuse *= diffuseTex.rgb;
        else
            diffuse = diffuseTex.rgb;
        alpha *= diffuseTex.a;
    }

    float3 normal;
    if (mat.bNormalTexture)
    {
        normal = texNormal.Sample(sampler0, input.texCoord) * 2.0 - 1.0;
        float3x3 tbn = float3x3(normalize(input.tangentVS), normalize(input.bitangentVS), normalize(input.normalVS));
        normal = normalize(mul(tbn, normal));
    }
    else
    {
        normal = normalize( input.normalVS );
    }

    float3 specular = mat.specular;
    if (mat.bSpecularTexture)
    {
        float3 specularTex = texSpecular.Sample( sampler0, input.texCoord );
        if (any(specular.rgb))
            specular *= specularTex;
        else
            specular = specularTex;
    }

    float3 emissive = mat.emissive;
    if (mat.bEmissiveTexture)
    {
        float3 emissiveTex = texEmissive.Sample( sampler0, input.texCoord ).rgb;
        if (any(emissive.rgb))
            emissive *= emissiveTex;
        else
            emissive = emissiveTex;
    }

    float3 ambient = mat.ambient;
    float3 position = input.positionVS;

    LightingResult lit = DoLighting( Lights, mat.specularStrength, position, normal );

    // Discard the alpha value from the lighting calculations.
    diffuse *= lit.Diffuse.rgb; 
    specular *= lit.Specular.rgb;
  
    float3 color = ambient + emissive + diffuse + specular;
    return float4(color, alpha);
}