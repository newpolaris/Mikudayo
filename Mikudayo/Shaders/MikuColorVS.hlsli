//
// Use code full.fx, AutoLuminous.fx
//

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
    float EdgeSize;
    float4 EdgeColor;
    float4 MaterialToon;
    int sphereOperation;
    int bUseTexture;
    int bUseToon;
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
static float3 LightDirection = normalize(SunDirectionWS);

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXTURE;
    float edgeScale : EDGE_SCALE;
};

static const int kSphereNone = 0;
static const int kSphereMul = 1;
static const int kSphereAdd = 2;

static float4 MaterialDiffuse = float4(Mat.diffuse, Mat.alpha);
static float3 MaterialAmbient = Mat.diffuse;
static float3 MaterialEmissive = Mat.ambient;
static float3 MaterialSpecular = Mat.specular;
static const float4 MaterialToon = Mat.MaterialToon;
static float3 LightDiffuse = float3(0,0,0);
static float3 LightAmbient = SunColor;
static float3 LightSpecular = SunColor;
#if !AUTOLUMINOUS
static float3 AutoLuminousColor = float3(0, 0, 0);
#else
static bool IsEmission = (100 < Mat.specularPower) && (length(MaterialSpecular) < 0.05);
static float3 AutoLuminousColor = (IsEmission ? MaterialDiffuse.rgb : float3(0, 0, 0));
#endif
static float4 DiffuseColor = MaterialDiffuse * float4(LightDiffuse, 1.0);
static float3 AmbientColor = saturate(MaterialAmbient * LightAmbient + MaterialEmissive + AutoLuminousColor);
static float3 SpecularColor = MaterialSpecular * LightSpecular;

