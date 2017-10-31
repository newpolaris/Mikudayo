cbuffer SkinningConstants : register(b1)
{
    matrix boneMatrix[1024];
}

float3 BoneSkinning( float3 position, float4 boneWeight, uint4 boneID )
{
    static const int kWeight = 4;
    float3 pos = float3(0, 0, 0);
    for (int i = 0; i < kWeight; i++)
	    pos += boneWeight[i] * mul( boneMatrix[boneID[i]], float4(position, 1.0) ).xyz;
    return pos;
}

float3 BoneSkinningNormal( float3 normal, float4 boneWeight, uint4 boneID )
{
    static const int kWeight = 4;
    float3 norm = float3(0, 0, 0);
    for (int i = 0; i < kWeight; i++)
	    norm += boneWeight[i] * mul( (float3x3)boneMatrix[boneID[i]], normal );
    return norm;
}
