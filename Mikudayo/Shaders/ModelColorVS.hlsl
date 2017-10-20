cbuffer VSConstants: register(b0)
{
	matrix view;
	matrix projection;
    matrix viewToShadow;
};

cbuffer Constants : register(b2)
{
	matrix model;
};

struct VertexShaderInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
	float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct PixelShaderInput
{
    float4 positionHS : SV_POSITION;
    float3 positionVS : POSITION;
    float2 texCoord : TEXCOORD0;
    float3 shadowCoord : TEXCOORD1;
    float3 normalVS : NORMAL;
    float3 tangentVS : TANGENT;
    float3 bitangentVS : BITANGENT;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;

    // Transform the vertex position into projected space.
	matrix modelview = mul( view, model );
    float4 positionWS = mul( model, float4( input.position, 1.0 ) );
	float4 positionVS = mul( view, positionWS );
	output.positionVS = positionVS.xyz;
	output.positionHS = mul( projection, positionVS );
	output.texCoord = input.texcoord;
    output.shadowCoord = mul(viewToShadow, positionWS).xyz;
	output.normalVS = input.normal;
    output.tangentVS = mul( (float3x3)modelview, input.tangent );
    output.bitangentVS = mul( (float3x3)modelview, input.bitangent );

	return output;
}
