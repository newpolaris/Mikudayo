// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXTURE;
};

cbuffer MaterialConstantBuffer : register(b0)
{
	float4 diffuse;
	float3 specular;
	float specularPower;
	float3 ambient;
};

Texture2D<float3>	texDiffuse		: register(t0);
Texture2D<float3>	texSpecular		: register(t1);
SamplerState		sampler0		: register(s0);

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	float3 color = texDiffuse.Sample( sampler0, input.uv );
	float3 lightDir = float3(1.f, 0.7f, 1.0f);
	float intensity = dot(lightDir, normalize(input.normal)) * 0.5f + 0.5f;
	return float4(color * intensity, 1.0) * diffuse;
}
