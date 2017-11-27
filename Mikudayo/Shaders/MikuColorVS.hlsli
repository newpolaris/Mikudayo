//
// Use code [Full]
// Use code [AutoLuminous]
//
#include "MikuColor.hlsli"

#define AUTOLUMINOUS 1

#if !AUTOLUMINOUS
static float3 AutoLuminousColor = float3(0, 0, 0);
#else
static bool IsEmission = (100 < Mat.specularPower) && (length(MaterialSpecular) < 0.05);
static float3 AutoLuminousColor = (IsEmission ? MaterialDiffuse.rgb : float3(0, 0, 0));
#endif
static float4 DiffuseColor = MaterialDiffuse * float4(LightDiffuse, 1.0);
static float3 AmbientColor = saturate(MaterialAmbient * LightAmbient + MaterialEmissive + AutoLuminousColor);
static float3 SpecularColor = MaterialSpecular * LightSpecular;

