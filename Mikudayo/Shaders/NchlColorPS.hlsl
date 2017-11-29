//
// Use code [NCHL]
//
#include "NchlColorVS.hlsli"
#include "Shadow.hlsli"
#define Toon 3

struct PixelShaderInput
{
    float4 positionCS : SV_POSITION;
    float3 positionWS : POSITION0;
    float3 eyeWS : POSITION1;
    float4 shadowPositionCS : POSITION2;
    float2 texCoord : TEXCOORD0;
    float3 normalWS : NORMAL;
};

struct PixelShaderOutput
{
    float4 color : SV_Target0;   // color pixel output (R11G11B10_FLOAT)
    float4 emissive : SV_Target1; // emissive color output (R11G11B10_FLOAT)
};

Texture2D<float4> texDiffuse : register(t1);
Texture2D<float4> texSphere : register(t2);
Texture2D<float4> texToon : register(t3);

Texture2D<float> texSSAO : register(t64);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

float3x3 ComputeTangentFrame(float3 Normal, float3 View, float2 UV)
{
    float3 dp1 = ddx(View);
    float3 dp2 = ddy(View);
    float2 duv1 = ddx(UV);
    float2 duv2 = ddy(UV);

    float3x3 M = float3x3(dp1, dp2, cross(dp1, dp2));
    float2x3 inverseM = float2x3(cross(M[1], M[2]), cross(M[2], M[0]));
    float3 Tangent = mul(float2(duv1.x, duv2.x), inverseM);
    float3 Binormal = mul(float2(duv1.y, duv2.y), inverseM);

    return float3x3(normalize(Tangent), normalize(Binormal), Normal);
}

// Beckmanormal distribution
float Beckmann(float m, float nh)
{
    m = max(0.0001, m);
    float nh2 = nh*nh;
    float m2 = m*m;
    return exp((nh2-1)/(nh2*m2)) / (PI*m2*nh2*nh2);
}

float Pow5(float n)
{
    return n*n*n*n*n;
}

PixelShaderOutput main(PixelShaderInput input)
{
    PixelShaderOutput output = (PixelShaderOutput)0;
    Material mat = Mat;

    uint2 pixelPos = input.positionCS.xy;
    float3 Vn = normalize(input.eyeWS);
    float3 Ln = normalize( -LightDirection );

    float specularMapVal = 1;
    float3 Nn = input.normalWS;
    bool useNormalMap = (mat.sphereOperation != kSphereNone);
    if (useNormalMap)
    {
        float3x3 tangentFrame = ComputeTangentFrame(input.normalWS, input.eyeWS, input.texCoord);
        float4 texColor = texSphere.Sample( sampler1, input.texCoord );
        Nn = mul(2.0*texColor.xyz - 1, tangentFrame);
        specularMapVal = texColor.a;
    }
    if (any(Nn))
        Nn = normalize(Nn);
    
    float rimPower = max(0, dot(Vn, -Ln));
    float NV = dot(Nn, Vn);

    bool face = MaterialDiffuse.a < 0.99 && NV < 0 ? 0 : 1;

    float3 ambientColor = AmbientColor;
    float4 diffuseColor = DiffuseColor;
    if (mat.bUseTexture) {
        float4 texColor = texDiffuse.Sample( sampler0, input.texCoord );
        ambientColor *= texColor.xyz;
        diffuseColor *= texColor;
    }

    // Halt Labert
	float LN = dot(Ln, Nn);
    float HLambert = LN*0.5 + 0.5;
   
    float ToonShade = smoothstep(SHADING_MIN, SHADING_MAX, HLambert);
    // Complete projection by doing division by w.
    float3 shadowCoord = input.shadowPositionCS.xyz / input.shadowPositionCS.w;
    float ShadowMapVal = 1.0;
    if (!any(saturate(shadowCoord.xy) != shadowCoord.xy))
    {
        shadowCoord = saturate( shadowCoord );
        ShadowMapVal = GetShadow( input.shadowPositionCS, input.positionCS.xyz );
        float lightIntensity = dot( Nn, -SunDirectionWS );
        ShadowMapVal = min( lightIntensity, ShadowMapVal );
    }
    float comp = lerp( ToonShade, ShadowMapVal*ToonShade, ShadowStrength*(1 - ShadowMapVal) );
    float3 diffuse = diffuseColor.xyz*comp;

    float ao = texSSAO[pixelPos];
    float3 aoColor = lerp(ao, sqrt(ao), 1-comp);
    aoColor = lerp(MaterialToon.xyz*diffuseColor.xyz, aoColor, aoColor);

    float Amblamb = dot(Nn, (cross( Vn, Ln )))*0.5 + 0.5;
    float3 AmbLight = lerp(AmbLightColor0*(Amblamb*Amblamb),AmbLightColor1*(1-Amblamb),1-Amblamb)*AmbLightPower.rrr;

    float SdN = dot( SKYDIR, Nn )*0.5f + 0.5f;
    float3 Hemisphere = lerp(GROUNDCOLOR, SKYCOLOR, SdN*SdN);

    float  BackHlamb = BackLightColor.r*(dot(-Ln,Nn)*0.5+0.5);
    float3 BackLight = BackLightColor*BackHlamb*BackHlamb*BackLightPower.rrr;

    float3 ambient = aoColor*(BackLight+(AmbLight*Hemisphere))*0.1;

    // specular
    float3 Hn = normalize(Vn + Ln);
    float  NH = dot(Nn, Hn);
    float  VH = dot(Vn, Hn);
    float D = Beckmann(ROUGHNESS, NH);
    float G = min( 1, min(2*NH*NV/VH, 2*NH*LN/VH) );
    float F = lerp( FRESNEL, 1, 1-Pow5(dot(Ln, Hn)) );
    float3 Specular = max(0, F*D*G/NV)*LightAmbient*ShadowMapVal*SPECULAR_EXTENT;
 
    float3 Gloss = float3(0,0,0);
    float3 Hn_I = normalize(Vn - Ln);
    float  NH_I = dot(Nn, Hn_I);
    float  VH_I = dot(Vn, Hn_I);
    float G_I = min( 1, min(-2*NH_I*NV/VH_I, -2*NH_I*LN/VH_I) );

    if ( Gloss_Type )  {
        Gloss = LightAmbient*ambient*(saturate(0.5-(1-G_I)*(2+G_I)))*(GLOSS_EXTENT-1);  
    } else if ( Enable_Gloss ) {
        Gloss = LightAmbient*ambient*GLOSS_TYPE;
    }

    ambient *= ambientColor;

    float Rim = saturate(1-NV*1.5);
    float3 RimLight = (Rim*lerp( comp*saturate(1 - BackHlamb)*BackLight, LightAmbient*SUBCOLOR*D, rimPower )*RIM_STRENGTH);

    float SpecularPower = mat.specularPower;
    float Sublamb = smoothstep( -0.3, 1.0, HLambert ) - smoothstep( 0.0, 1.1, HLambert );
    float3 Subsurface = diffuseColor.xyz*SUBCOLOR*(Sublamb.rrr*SUBDEPTH)*sqrt( Rim*0.5 + 0.5 );
    Subsurface *= lerp( SUBDEPTH / 100, 1, diffuse );

    float3 specularTerm = saturate( specularMapVal*(Specular + Gloss) + max( Subsurface, RimLight ) )*face;
    output.color = float4(ambient + diffuse + specularTerm, diffuseColor.a);

    return output;
}
