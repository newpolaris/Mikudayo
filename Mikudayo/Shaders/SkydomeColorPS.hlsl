Texture2D<float4> texDiffuse : register(t1);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);
SamplerState sampler3 : register(s3);

float4 main( float2 texcoord : TexCoord ) : SV_Target
{
    return texDiffuse.Sample(sampler3, texcoord );
}