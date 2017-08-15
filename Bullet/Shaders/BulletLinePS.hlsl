struct PixelShaderInput
{
	float4 PositionHS : SV_POSITION;
    float3 Color : COLOR;
};

float4 main(PixelShaderInput Input) : SV_TARGET
{
    return float4(Input.Color, 1.0);
}