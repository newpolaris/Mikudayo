#pragma once

#include "Mapping.h"
#include "Color.h"

class GraphicsContext;

namespace Utility
{
    void Initialize( void );
    void Shutdown( void );

    void DebugCube( GraphicsContext& Context, const Math::Matrix4& WorldToClip, void* VertexData, Color Color, bool bDepth = true );
    void DebugTexture( GraphicsContext& Context, D3D11_SRV_HANDLE SRV, LONG X = 0, LONG Y = 0, LONG W = 500, LONG H = 500 );
}