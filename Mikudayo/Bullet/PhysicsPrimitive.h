#pragma once

#include <memory>

#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "Math/Transform.h"
#include "IRigidBody.h"

class GraphicsContext;
class BaseRigidBody;
namespace Math
{
    class BoundingFrustum;
}

namespace Primitive
{
    using namespace Math;

    struct PhysicsPrimitiveInfo
    {
        ShapeType Type;
        float Mass;
        Vector3 Size;
        Vector3 Position;
        Quaternion Rotation;
    };

    class PhysicsPrimitive
    {
    public:
        PhysicsPrimitive();

        void Create( const PhysicsPrimitiveInfo& Info );
        void Destroy();
        void Draw( const Math::BoundingFrustum& CameraFrustum );
        void Update();

        ShapeType m_Type;
        ObjectType m_Kind;
        AffineTransform m_Transform;
        std::shared_ptr<BaseRigidBody> m_Body;
    };
    using PhysicsPrimitivePtr = std::shared_ptr<PhysicsPrimitive>;
    PhysicsPrimitivePtr CreatePhysicsPrimitive( const PhysicsPrimitiveInfo& Info );
}