// Copyright notice (2nd)
// Original Copyright notice
////////////////////////////////////////////////////////////////////////////////////////////////
//  FurShader
//     毛皮シェーダー
//		Program by T.Ogura
//		（MME/基本シェーダー製作 舞力介入P)
////////////////////////////////////////////////////////////////////////////////////////////////

#include "Skinning.hlsli"

struct Material
{
	float3 diffuse;
	float alpha; // diffuse alpha
	float3 specular;
	float specularPower;
	float3 ambient;
};

// vertex shader, geometry
cbuffer VSConstants : register(b0)
{
	matrix view;
	matrix projection;
};

cbuffer Model : register(b2)
{
	matrix model;
}

// pixel shader
cbuffer MaterialConstants : register(b3)
{
	Material material;
	int sphereOperation;
	int bUseTexture;
	int bUseToon;
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
Texture2D<float3> texFur : register(t3);

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
struct AttributeInput
{
	float3 normal : NORMAL;
	float2 uv : TEXTURE;
	uint4 boneID : BONE_ID;
	float4 boneWeight : BONE_WEIGHT;
	float boneFloat : EDGE_FLAT;
};

// Per-pixel color data passed through the pixel shader.
struct GeometryShaderInput
{
	float3 posW : POSITION0;
	float3 normalW : NORMAL;
	float2 uv : TEXTURE;
};

struct PixelShaderInput
{
	float4 posH : SV_POSITION;
	float3 posV : POSITION0;
	float3 normalV : NORMAL;
	float2 uv : TEXTURE;
};

struct FurPixelShaderInput
{
	float4 posH : SV_POSITION;
	float3 posV : POSITION0;
	float3 normalV : NORMAL;
	float2 uv : TEXTURE;
    uint fur : FUR_PARAM;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput vsBasic(AttributeInput input, float3 position : POSITION)
{
	PixelShaderInput output;

    float3 pos = BoneSkinning( position, input.boneWeight, input.boneID );
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

GeometryShaderInput vsFur(AttributeInput input, float3 position : POSITION)
{
	GeometryShaderInput output;

    float3 pos = BoneSkinning( position, input.boneWeight, input.boneID );
    float3 normal = BoneSkinningNormal( input.normal, input.boneWeight, input.boneID );
	output.posW = pos;
	output.uv = input.uv;
	output.normalW = normal;
	return output;
}

[maxvertexcount(30)]
void gsFur( triangle GeometryShaderInput input[3],
			uint primID : SV_PrimitiveID,
			inout TriangleStream<FurPixelShaderInput> Stream )
{
	matrix modelview = mul( view, model );

	const uint FurCount = 5;
	const float FurStep = 1/200.f;
    for (uint f = 0; f < FurCount; ++f) 
    {
        for (uint i = 0; i < 3; ++i) 
        {
            FurPixelShaderInput pin = (FurPixelShaderInput)0;
            float4 pos = float4(input[i].posW, 1.0);
			float3 normal = input[i].normalW;
            pin.uv = input[i].uv;
			pos.xyz += normalize(normal)*FurStep*(float)f;
            pos = mul( modelview, pos );
			pin.posV = pos.xyz;
	        pin.posH = mul( projection, pos );
            pin.normalV = mul( (float3x3)modelview, normal );
			pin.fur = f;
			Stream.Append(pin);
		}
		Stream.RestartStrip();
	}
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
    static const int kSphereNone = 0;
    static const int kSphereMul = 1;
    static const int kSphereAdd = 2;

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

float4 psBasicFur( FurPixelShaderInput input ) : SV_Target
{
    const int FurShellCount = 5; // FurShellの枚数
    const float FurSupecularPower = 2; // 毛の光る範囲
    const float2 FurFlowScale = float2(15,1); // 毛の流れる量
    const float3 FurColor = float3(0.8, 0.8, 0.8); // 毛の色

    const float3 lightAmbient = float3(1, 1, 1);
    const float3 lightSpecular = float3(1, 1, 1);
    const float3 materialEmmisive = float3(0, 0, 0);

	float3 diffuse = material.diffuse * SunColor;
	float3 ambient = saturate(material.ambient * lightAmbient + materialEmmisive);
	float3 specular = lightSpecular * material.specular;

	float3 lightVecV = normalize( -SunDirection );
	float3 normalV = normalize( input.normalV );

    float4 color = float4(ambient, 1.0);
    if (!bUseToon) {
        color.rgb += max( 0, dot( normalV, lightVecV ) ) * diffuse;
    }
    color.a = material.alpha;
    color = saturate( color );

	float3 toEyeV = -input.posV;
	float3 halfV = normalize( toEyeV + lightVecV );
	float NdotH = dot( normalV, halfV );
	specular = 1.0 - pow( max(NdotH, 0.001f), FurSupecularPower ) * float3(1, 1, 1);

    float fFur = (float)input.fur;
    float4 texColor = texDiffuse.Sample( samLinear, input.uv ) * color;
    float2 furDir = float2(1.0, 0.0);
    color.rgb = lerp( texColor.rgb, FurColor, specular.r); // Specular.rによってTexColor -> FurColorに変化させる
    // color.rgb = lerp( float3(0, 0, 0), FurColor, specular.r ); // 最初の版はこっち
    float2 uv = (input.uv - furDir / FurFlowScale * fFur);
    color.w = texFur.Sample( samLinear, uv ).r * (1.0 - fFur / (FurShellCount - 1.0));
    return color;
}

void psShadow( PixelShaderInput input )
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
DepthStencilState DepthTestOnMask {
	DepthEnable = True;	
	DepthWriteMask = Zero;
	DepthFunc = LESS_EQUAL;
};

RasterizerState RasterSolid {
	FillMode = Solid;
	CullMode = Back;
	FrontCounterClockwise = TRUE;
};

BlendState BlendShadow {
	BlendEnable[0] = False;
	RenderTargetWriteMask[0] = 0;
};

// シェーダ
VertexShader vs_main = CompileShader( vs_5_0, vsBasic() );
VertexShader vs_fur = CompileShader( vs_5_0, vsFur() );
GeometryShader gs_fur = CompileShader( gs_5_0, gsFur() );
PixelShader ps_main = CompileShader( ps_5_0, psBasic() );
PixelShader ps_fur = CompileShader( ps_5_0, psBasicFur() );
PixelShader ps_shadow = CompileShader( ps_5_0, psShadow() );

// テクニック
technique11 t0 {
	//パス
	pass p0 {
		// ステート設定
		SetBlendState( BlendOn, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthTestOn, 0 );
		SetRasterizerState( RasterSolid );
		
		// シェーダ
		SetVertexShader( vs_main );
		SetPixelShader( ps_main );
	}
	pass p1 {//Fur
		// ステート設定
		SetBlendState( BlendOn, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthTestOnMask, 0 );
		SetRasterizerState( RasterSolid );
		
		// シェーダ
		SetVertexShader( vs_fur );
		SetGeometryShader( gs_fur );
		SetPixelShader( ps_fur );
	}
}

technique11 shadow_cast {
	pass p0 {
		// ステート設定
		SetBlendState( BlendShadow, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthTestOn, 0 );
		SetRasterizerState( RasterSolid );

		SetVertexShader( vs_main );
		SetPixelShader( ps_shadow );
	}
}