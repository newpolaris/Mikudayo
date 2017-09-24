#define NumLights 12
#define PointLight 0
#define SpotLight 1
#define DirectionalLight 2

struct Material
{
	float3 diffuse;
	float alpha;
	float3 specular;
	float specularPower;
	float3 ambient;
};

struct Light
{
    float4 PositionWS;
    float4 PositionVS;
    float4 DirectionWS;
    float4 DirectionVS;
    float4 Color;
    float Range;
    uint Type;
    float SpotlightAngle;
    float _;
};

struct LightingResult
{
    float4 Diffuse;
    float4 Specular;
};

StructuredBuffer<Light> Lights : register(t5);

float4 DoDiffuse( Light light, float3 L, float3 N )
{
    float NdotL = max( dot( L, N ), 0 );
    return light.Color * NdotL;
}

float4 DoSpecular( Light light, Material mat, float3 V, float3 L, float3 N )
{
	float3 halfV = normalize( V + L );
	float NdotH = dot( N, halfV );
	return light.Color * pow( max(NdotH, 0.001f), mat.specularPower );
}

float DoAttenuation( Light light, float d )
{
    return 1.0f - smoothstep( light.Range * 0.75f, light.Range, d );
}

float DoSpotCone( Light light, float3 L )
{
    float minCos = cos( radians( light.SpotlightAngle ) );
    float maxCos = lerp( minCos, 1.0, 0.5 );
    float cosAngle = dot( light.DirectionVS.xyz, -L );
    return smoothstep( minCos, maxCos, cosAngle );
}

LightingResult DoPointLight( Light light, Material mat, float3 V, float3 P, float3 N )
{
    LightingResult ret;
    float3 L = P - light.PositionVS.xyz;
    float distance = length( L );
    L /= distance;
    float attenuation = DoAttenuation( light, distance );
    ret.Diffuse = DoDiffuse( light, L, N ) * attenuation;
    ret.Specular = DoSpecular( light, mat, V, L, N ) * attenuation;
    return ret;
}

LightingResult DoSpotLight( Light light, Material mat, float3 V, float3 P, float3 N )
{
    LightingResult ret;
    float3 L = P - light.PositionVS.xyz;
    float distance = length( L );
    L /= distance;
    float attenuation = DoAttenuation( light, distance );
    float spotIntensity = DoSpotCone( light, L );
    ret.Diffuse = DoDiffuse( light, L, N ) * attenuation * spotIntensity;
    ret.Specular = DoSpecular( light, mat, V, L, N ) * attenuation * spotIntensity;
    return ret;
}

LightingResult DoDirectionalLight( Light light, Material mat, float3 V, float3 N )
{
    LightingResult ret;
    float3 L = normalize(-light.DirectionVS.xyz);
    ret.Diffuse = DoDiffuse( light, L, N );
    ret.Specular = DoSpecular( light, mat, V, L, N );
    return ret;
}


LightingResult DoLighting( StructuredBuffer<Light> lights, Material mat, float3 P, float3 N )
{
    LightingResult ret = (LightingResult)0;
    float3 V = -P;
    for (int i = 0; i < NumLights; i++)
    {
        LightingResult result = (LightingResult)0;
        switch (lights[i].Type)
        {
        case PointLight:
            result = DoPointLight( lights[i], mat, V, P, N );
            break;
        case SpotLight:
            result = DoSpotLight( lights[i], mat, V, P, N );
            break;
        case DirectionalLight:
            result = DoDirectionalLight( lights[i], mat, V, N );
            break;
        }
        ret.Diffuse += result.Diffuse;
        ret.Specular += result.Specular;
    }
    return ret;
}