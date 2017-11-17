Texture2D<float4> texDiffuse : register(t1);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

// Use code from [Ray]
#define InvPIE 0.318309886142f

float2 ComputeSphereCoord( float3 normal )
{
    normal = clamp( normal, -1.0, 1.0 );
    float2 coord = float2((atan2( normal.x, normal.z ) * InvPIE * 0.5f + 0.5f), acos( normal.y ) * InvPIE);
    return coord;
}

float4 main( float2 texcoord : TexCoord ) : SV_Target
{
    return texDiffuse.Sample(sampler1, texcoord );
}