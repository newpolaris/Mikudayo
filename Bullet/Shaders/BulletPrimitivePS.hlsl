struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float3 Nor : NORMAL;
	float2 Tex : TEXCOORD;
	float4 WPos : WORLD_POSITION;
};

cbuffer CB0 : register(b0)
{
	float4 CameraPos;
};

cbuffer CB1 : register(b1)
{
	float4 Diffuse;
};

Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

float4 main( PS_INPUT input ) : SV_Target
{
	float4 tex = txDiffuse.Sample(samLinear, input.Tex);
	float3 light = normalize(float3(1.1, 0.9, -1));
	float3 nor = normalize(input.Nor);

	float l = dot(nor, light);
	if(l < 0)l = -l*0.1f;
	l = saturate(l*0.7 + 0.4);
	float3 eye = normalize(CameraPos.xyz - input.WPos.xyz);
	float3 h = normalize(light + eye);
	float s = pow(saturate(dot(nor, h)), 24)*0.2;

	float4 color = tex*float4(l*Diffuse.xyz, Diffuse.w);
	if (color.a <= 0.0)
        discard;
	color.xyz += s;
	return color;
}
