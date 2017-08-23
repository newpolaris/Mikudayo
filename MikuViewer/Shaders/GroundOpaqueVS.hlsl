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
    float4 shadowPosH[MaxSplit] : POSITION0;
};

void GetShadowData( float3 position, out float4 shadowPosH[MaxSplit] )
{
    for ( uint i = 0; i < MaxSplit; i++ )
    {
        matrix shadowTransform = shadow[i];
        shadowPosH[i] = mul( shadowTransform, float4(position, 1.0) );
    }
}


PixelShaderInput main( float3 pos : POSITION )
{
    matrix viewProjection = mul(projection, view);
    PixelShaderInput output;
	output.posH = mul(viewProjection, float4(pos, 1.0));
    GetShadowData( pos, output.shadowPosH );

    return output;
}