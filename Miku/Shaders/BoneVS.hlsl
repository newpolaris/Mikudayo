// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

cbuffer BoneWorldConstantBuffer : register(b1)
{
	matrix world;
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tangent : TANGENT;
	float2 uv : TEXTURE;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 normal : Normal;
	float2 uv : TEXTURE;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 1.0f);

	// Transform the vertex position into projected space.
	pos = mul(world, pos);
	pos = mul(view, pos);
	pos = mul(projection, pos);

	output.pos = pos;
	output.normal = mul(input.normal, (float3x3)model);
	output.uv = input.uv;

	return output;
}
