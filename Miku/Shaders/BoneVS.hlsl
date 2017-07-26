cbuffer VSConstants: register(b0)
{
	matrix view;
	matrix projection;
};

cbuffer BoneConstants : register(b1)
{
	matrix model;
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tangent : TANGENT;
	float2 uv : TEXTURE;
};

// Simple shader to do vertex processing on the GPU.
float4 main(VertexShaderInput input) : SV_POSITION
{
	float4 pos = float4(input.pos, 1.0f);
	return mul(mul(projection, mul(view, model)), pos);
}
