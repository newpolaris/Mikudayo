#include "CommonInclude.hlsli"
#include "Skinning.hlsli"

static const float edgeFactor = 1.5;

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
	float edgeScale : EDGE_SCALE;
    float3 position : POSITION;
};

#if 1
float4 main(VertexInput input) : SV_POSITION
{
    float3 pos = BoneSkinning( input.position, input.boneWeight, input.boneID );
    float3 normal = BoneSkinningNormal( input.normal, input.boneWeight, input.boneID );
    matrix toView = mul(view, model);
    float4 posVS = mul(toView, float4(pos, 1));
    float scale = length(posVS.xyz) / 1000 * edgeFactor;
    pos = pos + normal * EdgeSize * input.edgeScale * scale;
    matrix viewToClip = mul(projection, toView);
    return mul(viewToClip, float4(pos, 1.0));
}
#else
float4 main(VertexInput input) : SV_POSITION
{
    float3 pos = input.position + input.normal * EdgeSize * 0.01;
    matrix clipToProj = mul(projection, mul(view, model));
    pos = BoneSkinning( pos, input.boneWeight, input.boneID );
    return mul(clipToProj, float4(pos, 1.0));
}
#endif
