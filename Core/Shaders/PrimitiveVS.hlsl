cbuffer VSConstants : register(b0)
{
	matrix WorldToClip;
};

float4 main( float3 pos : POSITION ) : SV_POSITION
{
	return mul(WorldToClip, float4(pos, 1.0));
}