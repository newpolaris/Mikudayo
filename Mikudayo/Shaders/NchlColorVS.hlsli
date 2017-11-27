#include "MikuColor.hlsli"

// static const float4 DiffuseColor = float4(MaterialDiffuse.rgb*LightAmbient, saturate(MaterialDiffuse.a+0.01));
static const float4 DiffuseColor = MaterialDiffuse * float4(LightDiffuse, 1.0);

static const float  PI = 3.1415926;
static const float  AmbLightPower = 2;
static const float3 AmbColorXYZ = float3(90, 90, 100);
static const float3 AmbColorRxyz = float3(100, 80, 60);
static const float3 AmbientColor  = MaterialToon.xyz*MaterialEmissive;

static const float3 AmbLightColor0 = saturate(AmbColorXYZ*0.01); 
static const float3 AmbLightColor1 = saturate(AmbColorRxyz*1.8/PI); 

static const float3 BackLightPower = float3(1, 1, 1);
static const float3 BackLightXYZ = float3(85, 80, 100);
static const float3 BackLightColor = saturate(BackLightXYZ*0.01); 

static const float ShadowStrength = 0.6f;

#define SKYCOLOR float3(0.55,0.59,0.68)
#define GROUNDCOLOR float3(0.63,0.52,0.35)
#define SKYDIR float3(0.0,1.0,0.0)

#define SHADING_MIN 0.0   // -0.5〜0.0
#define SHADING_MAX 1.5   // 1.0〜2.0

#define ROUGHNESS MaterialSpecular.r*2
#define FRESNEL   MaterialSpecular.g*2

#define SPECULAR_EXTENT 0.1

#define GLOSS_EXTENT MaterialSpecular.b*2
#define GLOSS_TYPE comp*(saturate(0.5-(1-G)*(1+G_I)))*GLOSS_EXTENT*2

static bool Enable_Gloss = GLOSS_EXTENT != 0;
static bool Gloss_Type = GLOSS_EXTENT > 1;

#define RIM_STRENGTH 1.5

#define SUBDEPTH SpecularPower
#define SUBCOLOR MaterialToon.rgb
