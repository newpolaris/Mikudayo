struct Material
{
	float3 diffuse;
	float alpha;
	float3 specular;
	float specularPower;
	float3 ambient;
};

cbuffer MaterialConstants : register(b4)
{
    Material material;
    int sphereOperation;
    int bUseTexture;
    int bUseToon;
    float EdgeSize;
    float4 EdgeColor;
};
