
//----------------------------------------------------------------------------------
// Vertex shader that generates a full-screen triangle with texcoords
//----------------------------------------------------------------------------------
void main( 
    float4 position : POSITION,
    out float4 svPosition : SV_POSITION,
    inout float2 texcoord : TEXCOORD0 
)
{
    svPosition = position;
}
