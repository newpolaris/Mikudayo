#include "Diffuse.hlsli"

SamplerState LinearSampler : register(s0);
Texture2D<float3> SourceColor : register(t0);

float4 main( float4 position : SV_Position, float2 texcoord : TexCoords0 ) : SV_TARGET
{
	float3 color, sum = 0;
	float n = 0;

	[unroll]
	for (int i = -SAMP_NUM; i <= SAMP_NUM; i++) {
		float2 stex = texcoord + float2(SampStep.x * (float)i, 0);
		float e = exp(-pow((float)i / (SAMP_NUM / 2.0), 2) / 2);
		float3 orgColor = SourceColor.Sample(LinearSampler, stex);
		orgColor = pow( orgColor, 1 );
		sum += orgColor * e;
		n += e;
	}
	color = sum / n;
	return float4(color, 1);
}
