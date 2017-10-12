#pragma once

#include "IRigidBody.h"

class GraphicsContext;
namespace Math
{
    class AffineTransform;
    class Vector3;
    class Frustum;
}
namespace PrimitiveBatch
{
    using namespace Math;

    void Initialize();
    void Shutdown();
    void Append( ShapeType Type, const AffineTransform& Transform, const Vector3& Size, const Frustum& CameraFrustum );
    void Flush( GraphicsContext& gfxContext, const Math::Matrix4& WorldToClip );
}