cbuffer SkinningConstants : register(b1)
{
    matrix boneMatrix[256];
}

float3 BoneSkinning( float3 position, float4 boneWeight, uint4 boneID )
{
    const int kWeight = 4;
    float3 pos = float3(0, 0, 0);
    for (int i = 0; i < kWeight; i++)
	    pos += boneWeight[i] * mul( boneMatrix[boneID[i]], float4(position, 1.0) ).xyz;
    return pos;
}

float3 BoneSkinningNormal( float3 normal, float4 boneWeight, uint4 boneID )
{
    const int kWeight = 4;
    float3 norm = float3(0, 0, 0);
    for (int k = 0; k < kWeight; k++)
	    norm += boneWeight[k] * mul( (float3x3)boneMatrix[boneID[k]], normal );
    return norm;
}
