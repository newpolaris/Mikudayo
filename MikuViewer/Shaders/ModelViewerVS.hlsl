// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

cbuffer BoneConstantBuffer : register( b1 )
{
	matrix boneMatrix[256];
}

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float3 pos : POSITION;
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
	float3 posV : POSITION;
	float3 normalV : NORMAL;
	float2 uv : TEXTURE;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;

	float w0 = 1.0 - float(input.boneWeight) / 100.0f;
	float4 pos = float4(input.pos, 1.0f);
	float4 pos0 = mul( boneMatrix[input.boneID.x], pos );
	float4 pos1 = mul( boneMatrix[input.boneID.y], pos );
	pos = float4(lerp(pos0.xyz, pos1.xyz, w0), 1.0f);
	float3 normal0 = mul( (float3x3)boneMatrix[input.boneID.x], input.normal );
	float3 normal1 = mul( (float3x3)boneMatrix[input.boneID.y], input.normal );
	float3 normal = lerp( normal0, normal1, w0 );

	// Transform the vertex position into projected space.
	matrix modelview = mul( view, model );
	float4 posV = mul( modelview, pos );
	output.posV = posV.xyz; 
	output.posH = mul( projection, posV );
	output.normalV = mul( (float3x3)modelview, input.normal );
	output.uv = input.uv;

	return output;
}
