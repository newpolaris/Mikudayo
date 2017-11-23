RWTexture2D<float3> Velocity : register(u0);
Texture2D<float> DepthTexture : register(t0);

// Parameters required to convert screen space coordinates to world space params.
cbuffer ScreenToViewParams : register(b0)
{
    float4x4 Reprojection;
    float2 ScreenDimensions;
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    uint2 st = DTid.xy;
    float depth = DepthTexture.Load(int3(st, 0));
    float2 texCoord = st / ScreenDimensions;
    float4 currentCS = float4(float2(texCoord.x, 1-texCoord.y)*2 - 1, depth, 1.0f);
    float4 previousCS = mul( Reprojection, currentCS );
    previousCS /= previousCS.w;
    Velocity[DTid.xy] = float3((currentCS.xy - previousCS.xy) * 0.5, 0);
}