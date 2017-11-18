Texture2D<float4> texDiffuse : register(t1);

SamplerState sampler1 : register(s1);

float4 main( float2 texcoord : TexCoord ) : SV_Target
{
    return texDiffuse.Sample(sampler1, texcoord );
}