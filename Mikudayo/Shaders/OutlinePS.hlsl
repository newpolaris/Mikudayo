#include "CommonInclude.hlsli"

cbuffer MaterialConstants : register(b3)
{
    Material material;
    int sphereOperation;
    int bUseTexture;
    int bUseToon;
    float EdgeSize;
    float4 EdgeColor;
};

float4 main(float4 Position : SV_POSITION) : SV_TARGET
{	
    return EdgeColor;
}
