#include "CommonInclude.hlsli"
#include "Skinning.hlsli"

cbuffer VSConstants : register(b0)
{
	matrix viewToProjection;
};

cbuffer Model : register(b2)
{
	matrix model;
}

// Per-vertex data used as input to the vertex shader.
struct VertexInput
{
	float3 normal : NORMAL;
	float2 uv : TEXTURE;
	uint4 boneID : BONE_ID;
	float4 boneWeight : BONE_WEIGHT;
	float edgeScale : EDGE_SCALE;
    float3 position : POSITION;
};

// Simple shader to do vertex processing on the GPU.
float4 main(VertexInput input) : SV_POSITION
{
    float3 pos = input.position; // BoneSkinning( input.position, input.boneWeight, input.boneID );

    // Transform the vertex position into projected space.
    matrix modelToProjection = mul( viewToProjection, model );
	return mul( modelToProjection, float4(pos, 1.0) );
}
