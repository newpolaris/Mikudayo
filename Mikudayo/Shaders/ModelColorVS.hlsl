#define AUTOLUMINOUS 0

struct Material
{
    float3  diffuse;
    //-------------------------- ( 16 bytes )
    float3  specular;
    //-------------------------- ( 16 bytes )
    float3  ambient;
    //-------------------------- ( 16 bytes )
    float3  emissive;
    //-------------------------- ( 16 bytes )
    float3  transparent;
    float   padding0;
    //-------------------------- ( 16 bytes )
    float   opacity;
    float   shininess;
    float   specularStrength;
    bool    bDiffuseTexture;
    //-------------------------- ( 16 bytes )
    bool    bSpecularTexture;
    bool    bEmissiveTexture;
    bool    bNormalTexture;
    bool    bLightmapTexture;
    //-------------------------- ( 16 bytes )
    bool    bReflectionTexture;
    // float3  padding1;
    //--------------------------- ( 16 bytes )
};

cbuffer Constants: register(b0)
{
	matrix view;
	matrix projection;
    matrix viewToShadow;
    float3 cameraPosition;
};

cbuffer Constants : register(b2)
{
	matrix model;
};

cbuffer Material : register(b4)
{
    Material Mat;
};

cbuffer LightConstants : register(b5)
{
    float3 SunDirectionWS;
    float3 SunColor;
    float4 ShadowTexelSize;
}

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
    float3 eyeWS : POSITION0;
    float2 texCoord : TEXCOORD0;
    float3 normalWS : NORMAL;
    float4 color : COLOR0;
    float3 specular : COLOR1;
    float4 emissive : COLOR2;
};

static float4 MaterialDiffuse = float4(Mat.diffuse, Mat.opacity);
static float3 MaterialAmbient = Mat.ambient;
static float3 MaterialEmissive = Mat.emissive;
static float3 MaterialSpecular = Mat.specular;
static float3 LightDiffuse = float3(1, 1, 1);
static float3 LightAmbient = SunColor - 0.3;
static float3 LightSpecular = SunColor;
static float4 DiffuseColor = MaterialDiffuse * float4(LightDiffuse, 1.0);
static float3 AmbientColor  = MaterialAmbient  * LightAmbient + MaterialEmissive;
static float3 SpecularColor = MaterialSpecular * LightSpecular;
static bool IsEmission = (100 < Mat.shininess) && (length(MaterialSpecular) < 0.01);

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
    Material mat = Mat;

    // Transform the vertex position into projected space.
    matrix worldViewProjMatrix = mul( projection, mul( view, model ) );
    float4 position = float4(input.position, 1);
    output.positionHS = mul( worldViewProjMatrix, position );
    output.eyeWS = cameraPosition - mul( (float3x3)model, input.position );
    output.normalWS = normalize(mul( (float3x3)model, input.normal ));
    output.color.rgb = AmbientColor;
    output.color.rgb += max( 0, dot( output.normalWS, -SunDirectionWS ) ) * DiffuseColor.rgb;
    output.color.a = DiffuseColor.a;
#if 0
    output.color = saturate( output.color );
#else
    output.color = output.color;
#endif
#if !AUTOLUMINOUS
    output.emissive = float4(MaterialEmissive, MaterialDiffuse.a);
#else
    output.emissive = MaterialDiffuse;
    output.emissive.rgb += MaterialEmissive / 2;
    output.emissive.rgb *= 0.5;
    output.emissive.rgb = IsEmission ? output.emissive.rgb : float3(0, 0, 0);
#endif
	output.texCoord = input.texcoord;

    float3 halfVector = normalize( normalize( output.eyeWS ) + -SunDirectionWS );
    output.specular = pow( max( 0, dot( halfVector, output.normalWS ) ), mat.shininess ) * SpecularColor;
	return output;
}
