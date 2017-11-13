#include "pch.h"
#include "InputLayout.h"
#include "CommandContext.h"
#include "GpuBuffer.h"
#include "FullScreenTriangle.h"

namespace FullScreenTriangle {
    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT2 texcoord;
    };

    std::vector<InputDesc> InputLayout = {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D10_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    VertexBuffer m_Buffer;
}

void FullScreenTriangle::Create( void )
{
    Vertex vertices[3];

    vertices[0].position = XMFLOAT3( -1.0f, -1.0f, 1.0f );
    vertices[1].position = XMFLOAT3( -1.0f, 3.0f, 1.0f );
    vertices[2].position = XMFLOAT3( 3.0f, -1.0f, 1.0f );

    vertices[0].texcoord = XMFLOAT2( 0.0f, 1.0f );
    vertices[1].texcoord = XMFLOAT2( 0.0f, -1.0f );
    vertices[2].texcoord = XMFLOAT2( 2.0f, 1.0f );

    m_Buffer.Create( L"FullscreenTriangle", 3, sizeof( vertices[0] ), vertices );
}

void FullScreenTriangle::Clear( void )
{
    m_Buffer.Destroy();
}

void FullScreenTriangle::Draw( GraphicsContext& context ) {
    const UINT offset = 0;
    const UINT stride = sizeof( Vertex );
    context.SetVertexBuffer( 0, m_Buffer.VertexBufferView() );
    context.Draw( 3, 0 );
}