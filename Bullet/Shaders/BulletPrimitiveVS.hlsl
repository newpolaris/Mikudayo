struct VS_INPUT
{
	float4 Pos : POSITION;
	float3 Nor : NORMAL;
	float2 Tex : TEXCOORD;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float3 Nor : NORMAL;
	float2 Tex : TEXCOORD;
	float4 WPos : WORLD_POSITION;
};

cbuffer CB0 : register(b0)
{
	matrix mtxView;
	matrix mtxProj;
};

cbuffer CB1 : register(b1)
{
	matrix mtxWorld[64];
};

PS_INPUT main( VS_INPUT input, uint instanceID : SV_InstanceID )
{
	PS_INPUT output = (PS_INPUT)0;
	float4 pos = mul( mtxWorld[instanceID], input.Pos );
	output.WPos = pos;
	pos = mul( mtxView, pos );
	output.Pos = mul( mtxProj, pos );
	output.Nor = mul( (float3x3)mtxWorld[instanceID], input.Nor );
	output.Tex = input.Tex;
	return output;
}