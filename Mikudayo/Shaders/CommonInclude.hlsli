#define NumLights 128
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

StructuredBuffer<Light> Lights : register(t7);

float4 DoDiffuse( Light light, float3 L, float3 N )
{
    float NdotL = max( dot( L, N ), 0 );
    return light.Color * NdotL;
}

float4 DoSpecular( Light light, float specularPower, float3 V, float3 L, float3 N )
{
	float3 halfV = normalize( V + L );
	float NdotH = dot( N, halfV );
	return light.Color * pow( max(NdotH, 0.001f), specularPower );
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

LightingResult DoPointLight( Light light, float specularPower, float3 V, float3 P, float3 N )
{
    LightingResult ret;
    float3 L = light.PositionVS.xyz - P;
    float distance = length( L );
    L /= distance;
    float attenuation = DoAttenuation( light, distance );
    ret.Diffuse = DoDiffuse( light, L, N ) * attenuation;
    ret.Specular = DoSpecular( light, specularPower, V, L, N ) * attenuation;
    return ret;
}

LightingResult DoSpotLight( Light light, float specularPower, float3 V, float3 P, float3 N )
{
    LightingResult ret;
    float3 L = light.PositionVS.xyz - P;
    float distance = length( L );
    L /= distance;
    float attenuation = DoAttenuation( light, distance );
    float spotIntensity = DoSpotCone( light, L );
    ret.Diffuse = DoDiffuse( light, L, N ) * attenuation * spotIntensity;
    ret.Specular = DoSpecular( light, specularPower, V, L, N ) * attenuation * spotIntensity;
    return ret;
}

LightingResult DoDirectionalLight( Light light, float specularPower, float3 V, float3 N )
{
    LightingResult ret;
    float3 L = normalize(-light.DirectionVS.xyz);
    ret.Diffuse = DoDiffuse( light, L, N );
    ret.Specular = DoSpecular( light, specularPower, V, L, N );
    return ret;
}


LightingResult DoLighting( StructuredBuffer<Light> lights, float specularPower, float3 P, float3 N )
{
    LightingResult ret = (LightingResult)0;
    float3 V = -P;
    for (int i = 0; i < NumLights; i++)
    {
        // Skip point and spot lights that are out of range of the point being shaded.
        float3 dist = lights[i].PositionVS.xyz - P;
        bool bInside = dot(dist, dist) > lights[i].Range*lights[i].Range;
        if (lights[i].Type != DirectionalLight && bInside)
            continue;
        LightingResult result = (LightingResult)0;
        switch (lights[i].Type)
        {
        case PointLight:
            result = DoPointLight( lights[i], specularPower, V, P, N );
            break;
        case SpotLight:
            result = DoSpotLight( lights[i], specularPower, V, P, N );
            break;
        case DirectionalLight:
            result = DoDirectionalLight( lights[i], specularPower, V, N );
            break;
        }
        ret.Diffuse += result.Diffuse;
        ret.Specular += result.Specular;
    }
    return ret;
}

// Parameters required to convert screen space coordinates to view space params.
cbuffer ScreenToViewParams : register( b3 )
{
    float4x4 InverseProjection;
    float2 ScreenDimensions;
}

// Convert clip space coordinates to view space
float4 ClipToView( float4 clip )
{
    // View space position.
    float4 view = mul( InverseProjection, clip );
    // Perspecitive projection.
    view = view / view.w;

    return view;
}

// Convert screen space coordinates to view space.
float4 ScreenToView( float4 screen )
{
    // Convert to normalized texture coordinates
    float2 texCoord = screen.xy / ScreenDimensions;

    // Convert to clip space
    float4 clip = float4( float2( texCoord.x, 1.0f - texCoord.y ) * 2.0f - 1.0f, screen.z, screen.w );

    return ClipToView( clip );
}
