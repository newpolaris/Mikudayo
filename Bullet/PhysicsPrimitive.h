#pragma once

#include <memory>

#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "IRigidBody.h"

class GraphicsContext;
namespace Physics
{
    class BaseRigidBody;
}

namespace Primitive
{
    using namespace Math;
    using namespace Physics;

    extern void Initialize();
    extern void Shutdown();

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

        virtual void Create( const PhysicsPrimitiveInfo& Info );
        virtual void Destroy();
        virtual void Draw( GraphicsContext& gfxContext );

        ShapeType m_Type;
        ObjectType m_Kind;
        std::shared_ptr<BaseRigidBody> m_Body;
    };
    using PhysicsPrimitivePtr = std::shared_ptr<PhysicsPrimitive>;
    PhysicsPrimitivePtr CreatePhysicsPrimitive( const PhysicsPrimitiveInfo& Info );
}