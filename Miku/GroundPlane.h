#pragma once

#include "IRenderObject.h"
#include "InputLayout.h"
#include "GpuBuffer.h"

namespace Graphics
{
    extern std::vector<InputDesc> GroundPlanInputDesc;
    struct GroundPlane : public IRenderObject
    {
        GroundPlane();
        ~GroundPlane();
        void Clear();
        void Draw( GraphicsContext& gfxContext, eObjectFilter Filter );
        void Update( float ) {}

        VertexBuffer m_VertexBuffer;
        IndexBuffer m_IndexBuffer;
    };
}