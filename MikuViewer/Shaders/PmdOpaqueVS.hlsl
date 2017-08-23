#include "Skinning.hlsli"

static const uint MaxSplit = 4;

cbuffer VSConstants : register(b0)
{
	matrix view;
	matrix projection;
    matrix shadow[MaxSplit]; // T*P*V
};

cbuffer SkinningConstants : register(b1)
{
    SkinData skinData;
}

cbuffer Model : register(b2)
{
	matrix model;
}

// Per-vertex data used as input to the vertex shader.
struct AttributeInput
{
	float3 normal : NORMAL;
	float2 uv : TEXTURE;
	uint2 boneID : BONE_ID;
	uint boneWeight : BONE_WEIGHT;
	uint boneFloat : EDGE_FLAT;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 posH : SV_POSITION;
	float3 posV : POSITION0;
    float4 shadowPosH[MaxSplit] : POSITION1;
	float3 normalV : NORMAL;
	float2 uv : TEXTURE;
};

void GetShadowData( float3 position, out float4 shadowPosH[MaxSplit] )
{
    for ( uint i = 0; i < MaxSplit; i++ )
    {
        matrix shadowTransform = mul( shadow[i], model );
        shadowPosH[i] = mul( shadowTransform, float4(position, 1.0) );
    }
}

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(AttributeInput input, float3 position : POSITION)
{
	PixelShaderInput output;

    float3 pos, normal;
    PmdSkinInput skinInput = { position, input.normal, input.boneWeight, input.boneID };
    PmdSkinning( skinInput, skinData, pos, normal );

    // Transform the vertex position into projected space.
	matrix modelview = mul( view, model );
	float4 posV = mul( modelview, float4(pos, 1.0) );
	output.posV = posV.xyz;
	output.posH = mul( projection, posV );
	output.normalV = mul( (float3x3)modelview, normal );
	output.uv = input.uv;
    GetShadowData( pos, output.shadowPosH );

	return output;
}
