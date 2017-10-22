Texture2D<float3>	texColor        : register(t0);
SamplerState		sampler0		: register(s0);

float4 main(
	float4 Pos : SV_Position,
	float2 Tex : TexCoords0) : SV_TARGET
{
    float3 c = texColor.Sample( sampler0, Tex );
    return float4(c.rgb, 1.0f);
}
