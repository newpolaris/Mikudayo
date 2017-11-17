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

struct VertexShaderInput
{
    float3 position : POSITION;
    float3 normal : NORMAL; 
    float3 tangentU : TANGENTU;
    float2 texcoord : TEXCOORD;
};

struct PixelShaderInput
{
    float2 texcoord : TexCoord;
    float4 position : SV_Position;
};

PixelShaderInput main( VertexShaderInput input )
{
	PixelShaderInput output;
    matrix worldViewProjMatrix = mul( projection, mul( view, model ) );
    output.position = mul( worldViewProjMatrix, float4(input.position, 1) );
    output.texcoord = input.texcoord;
	return output;
}