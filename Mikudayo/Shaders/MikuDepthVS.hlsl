#include "MikuColorVS.hlsli"

// Simple shader to do vertex processing on the GPU.
float4 main(VertexShaderInput input) : SV_POSITION
{
    float3 pos = input.position;

    // Transform the vertex position into projected space.
    matrix modelToProjection = mul( projection, mul( view, model ) );
	return mul( modelToProjection, float4(pos, 1.0) );
}