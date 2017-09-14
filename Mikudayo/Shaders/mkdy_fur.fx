// Copyright notice (2nd)
/*
    Program source code are licensed under the zlib license, except source code of external library.

    Zerogram Sample Program
    http://zerogram.info/

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it freely,
    subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/
// Original Copyright notice (1st)
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
    float4 fur : FUR_PARAM;
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
    output.fur = float4(0, 0, 0, 1);

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
			inout TriangleStream<PixelShaderInput> Stream )
{
	matrix modelview = mul( view, model );
	const int FurCount = 10;
	const float FurStep = 0.003;
	for(int f=0;f<FurCount;++f){
		for(int i=0;i<3;++i){
			PixelShaderInput pin;
			float4 pos = float4(input[i].posW, 1.0);
			float3 normal = input[i].normalW;
			pos.xyz += normalize(normal)*FurStep*(float)f;
            pos = mul( modelview, pos );
			pin.posV = pos.xyz;
	        pin.posH = mul( projection, pos );
            pin.normalV = mul( (float3x3)modelview, normal );
			pin.fur = float4(0,0,0,((float)FurCount-(float)f-1)/(float)FurCount);
            pin.uv = input[i].uv;
			Stream.Append(pin);
		}
		Stream.RestartStrip();
	}
}


//----------------------------
float4 psBasicFur( PixelShaderInput input ) : SV_Target
{
	float3 lightVecV = normalize( -SunDirection );
	float3 normalV = normalize( input.normalV );
	float intensity = dot( lightVecV, normalV ) * 0.5 + 0.5;
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
	float3 color = texColor;
	float alpha = texAlpha * material.alpha;
    if (input.fur.w < 0.999f) 
    {
		float2 dir = float2(1,0)*(1.0-input.fur.w)*0.02;
		float2 bump = float2(0.002, 0.002)*input.fur.w;//バンプもどきで自己遮蔽っぽい
        float ss = (1.0 - input.fur.w)*(1.0 - texFur.Sample( samLinear, input.uv + bump + dir ).x);
		alpha *= input.fur.w*texFur.Sample( samLinear, input.uv + dir).x;
		color *= (1-0.3*ss);
	}
    color = color*((ambient + diffuse) + specular);
    return float4(color, alpha);
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
PixelShader ps_main = CompileShader( ps_5_0, psBasicFur() );
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
		SetPixelShader( ps_main );
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