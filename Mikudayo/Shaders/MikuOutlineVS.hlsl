#include "CommonInclude.hlsli"
#include "MikuColorVS.hlsli"

static const float edgeFactor = 1.5;

// Per-vertex data used as input to the vertex shader.
struct VertexInput
{
    float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXTURE;
	float edgeScale : EDGE_SCALE;
};

float4 main(VertexInput input) : SV_POSITION
{
    float3 pos = input.position;
    float3 normal = input.normal;
    matrix toView = mul(view, model);
    float4 posVS = mul(toView, float4(pos, 1));
    float scale = length(posVS.xyz) / 1000 * edgeFactor;
    pos = pos + normal * Mat.EdgeSize * input.edgeScale * scale;
    matrix viewToClip = mul(projection, toView);
    return mul(viewToClip, float4(pos, 1.0));
}