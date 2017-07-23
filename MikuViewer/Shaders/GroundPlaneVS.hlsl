cbuffer VSConstants : register(b0)
{
	matrix view;
	matrix projection;
    matrix shadow; // T*P*V
};

struct PixelShaderInput
{
	float4 posH : SV_POSITION;
    float4 shadowPosH : POSITION0;
};

PixelShaderInput main( float3 pos : POSITION )
{
    matrix viewProjection = mul(projection, view);
    PixelShaderInput output;
	output.posH = mul(viewProjection, float4(pos, 1.0));
    output.shadowPosH = mul(shadow, float4(pos, 1.0));
    return output;
}