cbuffer Constants: register(b0)
{
	matrix view;
	matrix projection;
    matrix viewToShadow;
    float3 cameraPosition;
};

cbuffer Model : register(b2)
{
	matrix model;
};

struct PixelShaderInput
{
    float3 normal : Normal;
    float4 position : SV_Position;
};

PixelShaderInput main( in float3 position : POSITION, in float3 normal : NORMAL, in float2 texcoord : TEXCOORD )
{
	PixelShaderInput output;
    matrix worldViewProjMatrix = mul( projection, mul( view, model ) );
    output.position = mul( worldViewProjMatrix, float4(position, 1) );
    output.normal = position;
	return output;
}