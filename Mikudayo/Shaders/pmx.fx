#include "Skinning.hlsli"

struct Material
{
    float3 diffuse;
    float alpha; // diffuse alpha
    float3 specular;
    float specularPower;
    float3 ambient;
};

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

static const int kSphereNone = 0;
static const int kSphereMul = 1;
static const int kSphereAdd = 2;

// pixel shader
cbuffer MaterialConstants : register(b3)
{
	Material material;
	int sphereOperation;
    bool bUseTexture;
    bool bUseToon;
    float EdgeSize;
    float4 EdgeColor;
};

cbuffer PassConstants : register(b4)
{
    float3 SunDirection; // V
    float3 SunColor;
    float4 ShadowTexelSize;
}

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float3> texSphere : register(t1);
Texture2D<float3> texToon : register(t2);

SamplerState samLinear : register(s0)
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
};

SamplerState samToon : register(s1)
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

SamplerState samSphere : register(s2)
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

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
struct PixelShaderInput
{
	float4 posH : SV_POSITION;
	float3 posV : POSITION0;
	float3 normalV : NORMAL;
	float2 uv : TEXTURE;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput vsBasic(VertexInput input)
{
	PixelShaderInput output;

    float3 pos = BoneSkinning( input.position, input.boneWeight, input.boneID );
    float3 normal = BoneSkinningNormal( input.normal, input.boneWeight, input.boneID );

    // Transform the vertex position into projected space.
	matrix modelview = mul( view, model );
	float4 posV = mul( modelview, float4(pos, 1.0) );
	output.posV = posV.xyz;
	output.posH = mul( projection, posV );
	output.normalV = mul( (float3x3)modelview, normal );
	output.uv = input.uv;

	return output;
}

float4 vsOutline(VertexInput input) : SV_POSITION
{
    float3 pos = input.position + input.normal * EdgeSize * 0.01;
    matrix clipToProj = mul(projection, mul(view, model));
    pos = BoneSkinning( pos, input.boneWeight, input.boneID );
    return mul(clipToProj, float4(pos, 1.0));
}

// A pass-through function for the (interpolated) color data.
float4 psBasic(PixelShaderInput input) : SV_TARGET
{
	float3 lightVecV = normalize( -SunDirection );
	float3 normalV = normalize( input.normalV );
	float intensity = dot( lightVecV, normalV ) * 0.5 + 0.5;
	float2 toonCoord = float2(0.5, 1.0 - intensity);

	float3 toEyeV = -input.posV;
	float3 halfV = normalize( toEyeV + lightVecV );

	float NdotH = dot( normalV, halfV );
	float specularFactor = pow( max(NdotH, 0.001f), material.specularPower );

	float3 diffuse = material.diffuse * SunColor;
	float3 ambient = material.ambient;
	float3 specular = specularFactor * material.specular;

	float texAlpha = 1.0;
	float3 texColor = float3(1.0, 1.0, 1.0);
	if (bUseTexture)
	{
		float4 tex = texDiffuse.Sample( samLinear, input.uv );
		texColor = tex.xyz;
		texAlpha = tex.w;
	}
	float2 sphereCoord = 0.5 + 0.5*float2(1.0, -1.0) * normalV.xy;
	if (sphereOperation == kSphereAdd)
		texColor += texSphere.Sample( samSphere, sphereCoord );
	else if (sphereOperation == kSphereMul)
		texColor *= texSphere.Sample( samSphere, sphereCoord );
	float3 color = texColor * (ambient + diffuse) + specular;
    if (bUseToon)
        color *= texToon.Sample( samToon, toonCoord );
	float alpha = texAlpha * material.alpha;
	return float4(color, alpha);
}

float4 psOutline(float4 Position : SV_POSITION) : SV_TARGET
{	
    return EdgeColor;
}

void psShadow(PixelShaderInput input)
{
	float3 diffuse = material.diffuse;
	float texAlpha = 1.0;
	if (bUseTexture)
		texAlpha = texDiffuse.Sample( samLinear, input.uv ).w;
    float alpha = material.alpha*texAlpha;
    clip( alpha - 0.001 );
}

BlendState NoBlend {
	BlendEnable[0] = False;
};
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
DepthStencilState DepthTestOff {
	DepthEnable = False;	
};

RasterizerState RasterSolid {
	FillMode = Solid;
	CullMode = Back;
	FrontCounterClockwise = True;
};
RasterizerState RasterOutline {
	FillMode = Solid;
	CullMode = Front;
	FrontCounterClockwise = True;
};
BlendState BlendShadow {
	BlendEnable[0] = False;
	RenderTargetWriteMask[0] = 0;
};
RasterizerState RasterShadow {
	FillMode = Solid;
	CullMode = Back;
	FrontCounterClockwise = True;
};

VertexShader vs_main = CompileShader( vs_5_0, vsBasic() );
VertexShader vs_outline = CompileShader( vs_5_0, vsOutline() );
PixelShader ps_main = CompileShader( ps_5_0, psBasic() );
PixelShader ps_outline = CompileShader( ps_5_0, psOutline() );
PixelShader ps_shadow = CompileShader( ps_5_0, psShadow() );

technique11 t0 {
	pass p0 {
		SetBlendState( BlendOn, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthTestOn, 0 );
		SetRasterizerState( RasterSolid );
		
		SetVertexShader( vs_main );
		SetPixelShader( ps_main );
	}
}

technique11 shadow_cast {
	pass p0 {
		SetBlendState( BlendShadow, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthTestOn, 0 );
		SetRasterizerState( RasterShadow );

		SetVertexShader( vs_main );
		SetPixelShader( ps_shadow );
	}
}

technique11 outline {
    pass p1 {
		SetDepthStencilState( DepthTestOn, 0 );
		SetRasterizerState( RasterOutline );

		SetVertexShader( vs_outline );
		SetPixelShader( ps_outline );
    }
}