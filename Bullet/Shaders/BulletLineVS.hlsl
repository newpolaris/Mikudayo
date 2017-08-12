cbuffer VSConstants : register(b0)
{
	matrix WorldToClip;
};

struct PixelShaderInput
{
	float4 PositionHS : SV_POSITION;
    float3 Color : COLOR;
};

PixelShaderInput main(float3 Position : POSITION, float3 Color : COLOR)
{
	PixelShaderInput output;
	output.PositionHS = mul( WorldToClip, float4(Position, 1.0));
    output.Color = Color;
	return output;
}