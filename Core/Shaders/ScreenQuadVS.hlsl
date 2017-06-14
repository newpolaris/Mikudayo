//----------------------------------------------------------------------------------
// Vertex shader that generates a full-screen triangle with texcoords
//----------------------------------------------------------------------------------
void main( 
	in uint VertID : SV_VertexID,
	out float4 Pos : SV_Position,
	out float2 Tex : TexCoords0
)
{
	Tex = float2( (VertID << 1) & 2, VertID & 2 );
    Pos = float4( Tex * float2( 2.0f, -2.0f ) + float2( -1.0f, 1.0f) , 0.0f, 1.0f );
}