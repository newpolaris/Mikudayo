#include "PixelPacking.hlsli"

RWTexture2D<float3> SceneColor : register( u0 );
Texture2D<uint> PostBuffer : register( t0 );

[numthreads( 8, 8, 1 )]
void main( uint3 DTid : SV_DispatchThreadID )
{
    SceneColor[DTid.xy] = Unpack_R11G11B10_FLOAT(PostBuffer[DTid.xy]);
}
