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

static const uint Bdef1 = 0;
static const uint Bdef2 = 1;
static const uint Bdef4 = 2;
static const uint Sdef = 3;
static const uint Qdef = 4;
static const uint elementSize = 52;

ByteAddressBuffer SkinUnit : register(t0);

cbuffer SkinningConstants : register(b0)
{
    float4 boneOrtho[1024][2];
}

float3 TransformBonePosition( float3 position, uint boneID )
{
    return TransformPosition( boneOrtho[boneID][0], boneOrtho[boneID][1], position );
}

float3 TransformBoneNormal( float3 noraml, uint boneIndex )
{
    return TransformNormal( boneOrtho[boneIndex][0], boneOrtho[boneIndex][1], noraml );
}

float2x4 BoneDualQuaternion( uint boneIndex )
{
    return TransformOrthToDualQuaternion( boneOrtho[boneIndex][0], boneOrtho[boneIndex][1] );
}

// Simple shader to do vertex processing on the GPU.
StreamOut main(VertexInput input, uint id : SV_VertexID )
{
    StreamOut output = (StreamOut)0;

    const uint baseOffset = id * elementSize;
    const uint type = SkinUnit.Load( baseOffset );
    if (type == Bdef1)
    {
        const uint boneID = SkinUnit.Load( baseOffset + 4 );
        output.position = TransformBonePosition( input.position, boneID );
        output.normal = TransformBoneNormal( input.normal, boneID );
    }
    else if (type == Bdef2)
    {
        const uint2 boneID = SkinUnit.Load2( baseOffset + 4 );
        const float weight1 = 1 - asfloat(SkinUnit.Load( baseOffset + 12 ));
	    float3 p0 = TransformBonePosition( input.position, boneID.x );
	    float3 p1 = TransformBonePosition( input.position, boneID.y );
	    float3 n0 = TransformBoneNormal( input.normal, boneID.x );
	    float3 n1 = TransformBoneNormal( input.normal, boneID.y );
        output.position = lerp( p0, p1, weight1 );
        output.normal = lerp( n0, n1, weight1 );
    }
    else if (type == Bdef4)
    {
        const uint4 boneID = SkinUnit.Load4( baseOffset + 4 );
        const float4 weight = asfloat(SkinUnit.Load4( baseOffset + 20 ));
        for (int i = 0; i < 4; i++)
        {
            output.position += weight[i] * TransformBonePosition( input.position, boneID[i] );
            output.normal += weight[i] * TransformBoneNormal( input.normal, boneID[i] );
        }
    }
    else if (type == Sdef)
    {
        const uint2 boneID = SkinUnit.Load2( baseOffset + 4 );
        const float weight1 = 1 - asfloat(SkinUnit.Load( baseOffset + 12 ));
        float2x4 dq0 = BoneDualQuaternion( boneID.x );
        float2x4 dq1 = BoneDualQuaternion( boneID.y );
        float2x4 blended = BlendedDualQuaternion2( dq0, dq1, weight1 );
        output.position = TransformPositionDualQuat( input.position, blended[0], blended[1] );
        output.normal = TransformNormalDualQuat( input.normal, blended[0], blended[1] );
    }
    return output;
}
