#pragma once

#include "IRigidBody.h"

class GraphicsContext;
namespace Math
{
    class AffineTransform;
    class Vector3;
}
namespace PrimitiveBatch
{
    void Initialize();
    void Shutdown();
    void Append( Physics::ShapeType Type, const Math::AffineTransform& Transform, const Math::Vector3& Size );
    void Flush( GraphicsContext& UiContext );
}