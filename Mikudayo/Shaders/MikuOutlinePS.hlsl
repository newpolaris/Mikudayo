#include "CommonInclude.hlsli"
#include "MikuColorVS.hlsli"

float4 main(float4 Position : SV_POSITION) : SV_TARGET
{	
    return Mat.EdgeColor;
}