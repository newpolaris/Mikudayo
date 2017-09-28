#include "CommonInclude.hlsli"
#include "Skinning.hlsli"

cbuffer VSConstants : register(b0)
{
	matrix view;
	matrix projection;
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
	float boneFloat : EDGE_FLAT;
    float3 position : POSITION;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 posH : SV_POSITION;
	float3 posV : POSITION0;
	float3 normalV : NORMAL;
	float2 uv : TEXTURE;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(VertexInput input)
{
	PixelShaderInput output;

    float3 pos = BoneSkinning( input.position, input.boneWeight, input.boneID );
    float3 normal = BoneSkinningNormal( input.normal, input.boneWeight, input.boneID );

    // Transform the vertex position into projected space.
	matrix modelview = mul( view, model );
	float4 posV = mul( modelview, float4(pos, 1.0) );
	output.posV = posV.xyz;
	output.posH = mul( projection, posV );
	output.normalV = mul( (float3x3)modelview, normal );
	output.uv = input.uv;

	return output;
}
