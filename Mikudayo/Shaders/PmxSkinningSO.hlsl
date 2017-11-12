#include "CommonInclude.hlsli"
#include "Skinning.hlsli"

// Per-vertex data used as input to the vertex shader.
struct VertexInput
{
    float3 position : POSITION;
	float3 normal : NORMAL;
};

struct StreamOut
{
    float3 position : POSITION;
	float3 normal : NORMAL;
};

// Simple shader to do vertex processing on the GPU.
StreamOut main(VertexInput input, uint id : SV_VertexID )
{
    StreamOut output = (StreamOut)0;

    float3 normal = input.normal; // BoneSkinningNormal( input.normal, input.boneWeight, input.boneID );
    output.position = Skinning( input.position, id );
    output.normal = normal;
    return output;
}
