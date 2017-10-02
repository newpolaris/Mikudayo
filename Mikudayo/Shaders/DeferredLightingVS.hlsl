#include "CommonInclude.hlsli"

cbuffer Constant : register(b2)
{
	matrix modelToClip;
}

// Simple shader to do vertex processing on the GPU.
float4 main(float3 position : POSITION) : SV_POSITION
{
	return mul( modelToClip, float4(position, 1.0) );
}
