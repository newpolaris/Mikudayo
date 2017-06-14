Texture2D ColorTex : register(t0);

float4 main( float4 Pos : SV_Position ) : SV_TARGET
{
	return ColorTex[(int2)Pos.xy];
}
