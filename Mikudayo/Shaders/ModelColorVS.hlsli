#define AUTOLUMINOUS 1

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
    float3  transparent;
    float   padding0;
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
    // float3  padding1;
    //--------------------------- ( 16 bytes )
};

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

cbuffer Material : register(b4)
{
    Material Mat;
};

cbuffer LightConstants : register(b5)
{
    float3 SunDirectionWS;
    float3 SunColor;
    float4 ShadowTexelSize;
}

struct VertexShaderInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
	float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct PixelShaderInput
{
    float4 positionHS : SV_POSITION;
    float4 positionCS : POSITION1;
    float3 positionWS : POSITION2;
    float3 eyeWS : POSITION0;
    float2 texCoord : TEXCOORD0;
    float3 normalWS : NORMAL;
    float4 color : COLOR0;
    float3 specular : COLOR1;
    float4 emissive : COLOR2;
};

static float4 MaterialDiffuse = float4(Mat.diffuse, Mat.opacity);
static float3 MaterialAmbient = Mat.ambient;
static float3 MaterialEmissive = Mat.emissive;
static float3 MaterialSpecular = Mat.specular;
static float3 LightDiffuse = float3(1, 1, 1);
static float3 LightAmbient = SunColor - 0.3;
static float3 LightSpecular = SunColor;
static bool IsEmission = (100 < Mat.shininess) && (length(MaterialSpecular) < 0.05);
#if !AUTOLUMINOUS
static float3 AutoLuminousColor = float3(0, 0, 0);
#else
static float3 AutoLuminousColor = (IsEmission ? MaterialDiffuse.rgb : float3(0, 0, 0));
#endif
static float4 DiffuseColor = MaterialDiffuse * float4(LightDiffuse, 1.0);
static float3 AmbientColor = MaterialAmbient * LightAmbient + MaterialEmissive + AutoLuminousColor;
static float3 SpecularColor = MaterialSpecular * LightSpecular;
