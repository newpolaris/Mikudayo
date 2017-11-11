#include "CommonInclude.hlsli"
#include "Skinning.hlsli"

// Per-vertex data used as input to the vertex shader.
struct VertexInput
{
    float3 position : POSITION;
	float3 normal : NORMAL;
	uint4 boneID : BONE_ID;
	float4 boneWeight : BONE_WEIGHT;
};

struct StreamOut
{
    float3 position : POSITION;
	float3 normal : NORMAL;
};

// Simple shader to do vertex processing on the GPU.
StreamOut main(VertexInput input)
{
    StreamOut output = (StreamOut)0;
    float3 pos = BoneSkinning( input.position, input.boneWeight, input.boneID );
    float3 normal = BoneSkinningNormal( input.normal, input.boneWeight, input.boneID );
    output.position = pos;
    output.normal = normal;
    return output;
}
