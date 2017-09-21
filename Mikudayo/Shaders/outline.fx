#include "Skinning.hlsli"

// vertex shader
cbuffer VSConstants : register(b0)
{
	matrix view;
	matrix projection;
};

cbuffer Model : register(b2)
{
	matrix model;
}

// Per-vertex data used as input to the vertex shader.
struct VertexInput
{
	float3 normal : NORMAL;
	float2 uv : TEXTURE;
	uint4 boneID : BONE_ID;
	float4 boneWeight : BONE_WEIGHT;
	float edgeScale : EDGE_FLAT;
    float3 position : POSITION;
};

// Per-pixel color data passed through the pixel shader.
struct GeometryInput
{
	float4 PosHS : SV_POSITION;
    float Edge : EDGE_FLAT;
};

struct PixelInput
{
	float4 PosHS : SV_POSITION;
    float Edge : EDGE_FLAT;
};

// Simple shader to do vertex processing on the GPU.
GeometryInput vsBasic(VertexInput input)
{
	GeometryInput output;

    float3 pos = BoneSkinning( input.position, input.boneWeight, input.boneID );

    // Transform the vertex position into projected space.
	matrix modelview = mul( view, model );
	float4 posVS = mul( modelview, float4(pos, 1.0) );
	output.PosHS = mul( projection, posVS );
    output.Edge = input.edgeScale;

	return output;
}

[maxvertexcount(10)]
void gsBasic(
    lineadj GeometryInput input[4], 
    uint primID : SV_PrimitiveID,
    inout LineStream<PixelInput> Stream )
{
	float3 v0 = input[1].PosHS.xyz/input[1].PosHS.w;
	float3 v1 = input[2].PosHS.xyz/input[2].PosHS.w;
	float3 va = input[0].PosHS.xyz/input[0].PosHS.w;
	float3 vb = input[3].PosHS.xyz/input[3].PosHS.w;
	
	float3 v0a = normalize(v0-va);
	float3 v1a = normalize(v1-va);
	float3 v0b = normalize(v0-vb);
	float3 v1b = normalize(v1-vb);
	if( -0.0001 > cross(v0a,v1a).z * cross(v1b,v0b).z )
	{
		PixelInput vo0,vo1;
		float4 p0 = input[1].PosHS;
		float4 p1 = input[2].PosHS;
		vo0.PosHS = p0;
		vo1.PosHS = p1;
		vo0.Edge = input[1].Edge;
		vo1.Edge = input[2].Edge;
		Stream.Append(vo0);
		Stream.Append(vo1);
		Stream.RestartStrip();

    #if 1
		const float px = 0.8*p0.w/640.0;
		const float py = 0.8*p1.w/480.0;

		vo0.PosHS = p0 + float4(px,0,0,0);
		vo1.PosHS = p1 + float4(px,0,0,0);
		Stream.Append(vo0);
		Stream.Append(vo1);
		Stream.RestartStrip();

		vo0.PosHS = p0 + float4(-px,0,0,0);
		vo1.PosHS = p1 + float4(-px,0,0,0);
		Stream.Append(vo0);
		Stream.Append(vo1);
		Stream.RestartStrip();
		
		vo0.PosHS = p0 + float4(0,py,0,0);
		vo1.PosHS = p1 + float4(0,py,0,0);
		Stream.Append(vo0);
		Stream.Append(vo1);
		Stream.RestartStrip();

		vo0.PosHS = p0 + float4(0,-py,0,0);
		vo1.PosHS = p1 + float4(0,-py,0,0);
		Stream.Append(vo0);
		Stream.Append(vo1);
		Stream.RestartStrip();
    #endif
	}

	Stream.RestartStrip();
}

float4 psBasic( PixelInput input ) : SV_TARGET
{	
    return float4(0,0,0,0.25*input.Edge);
}

BlendState BlendOn {
	BlendEnable[0] = True;
	BlendOp[0] = ADD;
	SrcBlend[0] = SRC_ALPHA;
	DestBlend[0] = INV_SRC_ALPHA;
	SrcBlendAlpha[0] = ONE;
	DestBlendAlpha[0] = ZERO;
	BlendOpAlpha[0] = ADD;
};
DepthStencilState DepthTestOn {
	DepthEnable = True;	
	DepthWriteMask = All;
	DepthFunc = LESS_EQUAL;
};
RasterizerState RasterOutline {
    AntialiasedLineEnable = true;
	FillMode = Solid;
	CullMode = Back;
	FrontCounterClockwise = True;
};

VertexShader vs_main = CompileShader( vs_5_0, vsBasic() );
GeometryShader gs_main = CompileShader( gs_5_0, gsBasic() );
PixelShader ps_main = CompileShader( ps_5_0, psBasic() );

technique11 t0 {
    pass p0 {
		SetBlendState( BlendOn, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthTestOn, 0 );
		SetRasterizerState( RasterOutline );

		SetVertexShader( vs_main );
		SetGeometryShader( gs_main );
		SetPixelShader( ps_main );
	}
}

