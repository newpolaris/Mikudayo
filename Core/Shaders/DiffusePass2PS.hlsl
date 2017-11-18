#include "Diffuse.hlsli"

// Use code from [Diffuse]

SamplerState LinearSampler : register(s0);
Texture2D<float3> SourceColor : register(t0);
Texture2D<float3> OriginColor : register(t1);

float3 BrightnessCompare(float3 color1, float3 color2)
{
#if BCOMP_TYPE==0
	float brightness1 = (color1.r * 0.29 + color1.g * 0.59 + color1.b * 0.12);
	float brightness2 = (color2.r * 0.29 + color2.g * 0.59 + color2.b * 0.12 );

	if (brightness2 > brightness1) color1 = color2;

#elif BCOMP_TYPE==1
	float brightness1 = length(color1.rgb);
	float brightness2 = length(color2.rgb);

	if (brightness2 > brightness1) color1 = color2;

#elif BCOMP_TYPE==2
	float brightness1 = (color1.r + color1.g + color1.b );
	float brightness2 = (color2.r + color2.g + color2.b );

	if (brightness2 > brightness1) color1 = color2;

#elif BCOMP_TYPE==3
	color1.r = color2.r > color1.r ? color2.r : color1.r;
	color1.g = color2.g > color1.g ? color2.g : color1.g;
	color1.b = color2.b > color1.b ? color2.b : color1.b;
#endif
	return color1;
}

float4 main( float4 position : SV_POSITION, float2 texcoord : TEXCOORD0 ) : SV_TARGET
{
	float3 color, sum = 0;
	float3 colorSrc, colorOrg;
	float n = 0;

    [unroll]
	for(int i = -SAMP_NUM; i <= SAMP_NUM; i++) {
        float2 stex = texcoord + float2(0, SampStep.y * (float)i);
        float e = exp(-pow((float)i / (SAMP_NUM / 2.0), 2) / 2);
		sum += SourceColor.Sample(LinearSampler, stex) * e;
        n += e;
    }
    
    color = sum / n;
    colorOrg = OriginColor.Sample(LinearSampler, texcoord );
    colorSrc = pow( colorOrg, 2 );
    
    color = color + colorSrc - color * colorSrc;
    
    #if MIX_TYPE==0
        color = BrightnessCompare(color, colorOrg);
    #else
        color = BrightnessCompare(color, colorSrc);
    #endif
    
    color = lerp(colorOrg, color, Strength);
    
	return saturate(float4(color, 1));
}
