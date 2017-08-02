static const uint MaxSplit = 4;

cbuffer VSConstants : register(b0)
{
	matrix view;
	matrix projection;
    matrix shadow[MaxSplit]; // T*P*V
};

struct PixelShaderInput
{
	float4 posH : SV_POSITION;
    float3 posW : POSITION1;
	float3 normalW : NORMAL1;
    float depthV : DEPTH0;
};

PixelShaderInput main( float3 pos : POSITION )
{
    matrix viewProjection = mul(projection, view);
    PixelShaderInput output;
	output.posW = pos;
	output.posH = mul(viewProjection, float4(pos, 1.0));
    output.normalW = float3(0, 1, 0);
	output.depthV = output.posH.w;

    return output;
}