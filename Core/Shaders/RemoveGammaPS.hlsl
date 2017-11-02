#include "ShaderUtility.hlsli"

Texture2D<float3> SrcColor : register(t0);

float3 main( float4 position : SV_Position ) : SV_Target0
{
	return RemoveDisplayProfile(SrcColor[(int2)position.xy], COLOR_FORMAT_sRGB_FULL);
}