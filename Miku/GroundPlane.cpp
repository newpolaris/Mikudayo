#include "GroundPlane.h"
#include "InputLayout.h"
#include "CommandContext.h"

namespace Graphics
{
    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT3 normal;
    };

    std::vector<InputDesc> GroundPlanInputDesc
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
}

using namespace Graphics;
using namespace Math;

GroundPlane::GroundPlane()
{
    const static float w = 100.0f;
    Vertex vertices[] =
    {
        { XMFLOAT3( -w, 0,  w ), XMFLOAT3( 0.0f, 1.0f, 0.0f ) },
        { XMFLOAT3( w, 0,  w ), XMFLOAT3( 0.0f, 1.0f, 0.0f ) },
        { XMFLOAT3( w, 0, -w ), XMFLOAT3( 0.0f, 1.0f, 0.0f ) },
        { XMFLOAT3( -w, 0, -w ), XMFLOAT3( 0.0f, 1.0f, 0.0f ) },
    };
    m_VertexBuffer.Create( L"GroundVB", _countof( vertices ), sizeof( Vertex ), vertices );

    uint16_t indices[] =
    {
        0,1,2,
        0,2,3,
    };
    m_IndexBuffer.Create( L"GroundIB", _countof( indices ), sizeof( uint16_t ), indices );
}

GroundPlane::~GroundPlane()
{
    Clear();
}

void GroundPlane::Clear()
{
    m_VertexBuffer.Destroy();
    m_IndexBuffer.Destroy();
}

void GroundPlane::Draw( GraphicsContext& gfxContext, eObjectFilter Filter )
{
    if (Filter & kOpaque)
    {
        gfxContext.SetVertexBuffer( 0, m_VertexBuffer.VertexBufferView() );
        gfxContext.SetIndexBuffer( m_IndexBuffer.IndexBufferView() );
        gfxContext.DrawIndexed( 6 );
    }
}

BoundingBox GroundPlane::GetBoundingBox()
{
    return BoundingBox( Vector3( 0.f ), Vector3( 0.f ) );
}
