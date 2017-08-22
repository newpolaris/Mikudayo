#include "Skinning.hlsli"

cbuffer VSConstants : register(b0)
{
	matrix view;
	matrix projection;
    matrix shadow; // T*P*V
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
	uint4 boneID : BONE_ID;
	float4 boneWeight : BONE_WEIGHT;
	float boneFloat : EDGE_FLAT;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 posH : SV_POSITION;
	float2 uv : TEXTURE;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(AttributeInput input, float3 position : POSITION)
{
	PixelShaderInput output;

    // normal is not used depth write
    float3 pos, normal;
    PmxSkinInput skinInput = { position, input.normal, input.boneWeight, input.boneID };
    PmxSkinning( skinInput, skinData, pos, normal );

    // Transform the vertex position into projected space.
	matrix modelview = mul( view, model );
	float4 posV = mul( modelview, float4(pos, 1.0) );
	output.posH = mul( projection, posV );
	output.uv = input.uv;

	return output;
}
