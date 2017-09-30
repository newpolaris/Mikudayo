#include "CommonInclude.hlsli"

cbuffer VSConstants : register(b0)
{
	matrix view;
	matrix projection;
};

cbuffer Model : register(b2)
{
	matrix model;
}

// Simple shader to do vertex processing on the GPU.
float4 main(float3 position : POSITION) : SV_POSITION
{
	matrix modelToClip = mul( projection, mul( view, model ) );
	return mul( modelToClip, float4(position, 1.0) );
}
