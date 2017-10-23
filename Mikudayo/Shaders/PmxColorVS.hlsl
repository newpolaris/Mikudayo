#include "CommonInclude.hlsli"
#include "Skinning.hlsli"

#define AUTOLUMINOUS 1

cbuffer Constants: register(b0)
{
	matrix view;
	matrix projection;
    matrix viewToShadow;
    float3 cameraPosition;
};

cbuffer Model : register(b2)
{
	matrix model;
};

struct Material
{
	float3 diffuse;
	float alpha;
	float3 specular;
	float specularPower;
	float3 ambient;
    int sphereOperation;
    int bUseTexture;
    int bUseToon;
    float EdgeSize;
    float4 EdgeColor;
};

cbuffer MaterialConstants : register(b4)
{
    Material Mat;
};

cbuffer LightConstants : register(b5)
{
    float3 SunDirectionWS;
    float3 SunColor;
    float4 ShadowTexelSize;
}

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float3 normal : NORMAL;
	float2 texcoord : TEXTURE;
	uint4 boneID : BONE_ID;
	float4 boneWeight : BONE_WEIGHT;
	float edgeScale : EDGE_SCALE;
    float3 position : POSITION;
};

struct PixelShaderInput
{
    float4 positionHS : SV_POSITION;
    float3 eyeWS : POSITION0;
    float2 texCoord : TEXCOORD0;
    float2 spTex : TEXCOORD1;
    float3 normalWS : NORMAL;
    float4 color : COLOR0;
    float3 specular : COLOR1;
    float4 emissive : COLOR2;
};

static const int kSphereNone = 0;
static const int kSphereMul = 1;
static const int kSphereAdd = 2;

static float4 MaterialDiffuse = float4(Mat.diffuse, Mat.alpha);
static float3 MaterialAmbient = Mat.diffuse;
static float3 MaterialEmissive = Mat.ambient;
static float3 MaterialSpecular = Mat.specular;
static float3 LightDiffuse = SunColor;
static float3 LightAmbient = SunColor;
static float3 LightSpecular = SunColor;
#if !AUTOLUMINOUS
static float3 AutoLuminousColor = float3(0, 0, 0);
#else
static bool IsEmission = (100 < Mat.specularPower) && (length(MaterialSpecular) < 0.05);
static float3 AutoLuminousColor = (IsEmission ? MaterialDiffuse.rgb : float3(0, 0, 0));
#endif
static float4 DiffuseColor = MaterialDiffuse * float4(LightDiffuse, 1.0);
static float3 AmbientColor = MaterialAmbient * LightAmbient + MaterialEmissive + AutoLuminousColor;
static float3 SpecularColor = MaterialSpecular * LightSpecular;

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output = (PixelShaderInput)0;
    Material mat = Mat;

    float3 position = BoneSkinning( input.position, input.boneWeight, input.boneID );
    float3 normal = BoneSkinningNormal( input.normal, input.boneWeight, input.boneID );

    // Transform the vertex position into projected space.
    matrix worldViewProjMatrix = mul( projection, mul( view, model ) );
    output.positionHS = mul( worldViewProjMatrix, float4(position, 1) );
    output.eyeWS = cameraPosition - mul( (float3x3)model, position );
    output.normalWS = normalize(mul( (float3x3)model, normal ));
    output.color.rgb = AmbientColor;
    if (!Mat.bUseToon) {
        output.color.rgb += max( 0, dot( output.normalWS, -SunDirectionWS ) ) * DiffuseColor.rgb;
    }
    output.color.a = DiffuseColor.a;
    output.emissive = float4(MaterialEmissive, MaterialDiffuse.a);
	output.texCoord = input.texcoord;
    if ( Mat.sphereOperation != kSphereNone ) {
        float2 normalVS = mul( (float3x3)view, output.normalWS ).xy;
        output.spTex.x = normalVS.x * 0.5 + 0.5;
        output.spTex.y = normalVS.y * -0.5 + 0.5;
    }
    float3 halfVector = normalize( normalize( output.eyeWS ) + -SunDirectionWS );
    output.specular = pow( max( 0, dot( halfVector, output.normalWS ) ), mat.specularPower ) * SpecularColor;
#if AUTOLUMINOUS
    if (IsEmission)
    {
        output.emissive.rgb += MaterialDiffuse.rgb;
        // from Autoluminous 'EmittionPower0'
        float factor = max( 1, (mat.specularPower - 100) / 7 );
        output.emissive.rgb *= factor*2;
        output.color.rgb *= factor*2;
    }
#endif
	return output;
}
