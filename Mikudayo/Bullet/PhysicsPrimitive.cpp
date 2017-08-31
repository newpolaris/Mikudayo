#include "stdafx.h"
#include "LinearMath.h"
#include "PhysicsPrimitive.h"
#include "Physics.h"
#include "BaseRigidBody.h"
#include "PrimitiveBatch.h"

using namespace Physics;
using namespace Primitive;

PhysicsPrimitivePtr Primitive::CreatePhysicsPrimitive( const PhysicsPrimitiveInfo& Info )
{
    auto Ptr = std::make_shared<PhysicsPrimitive>();
    if (Ptr)
        Ptr->Create( Info );
    return Ptr;
}

PhysicsPrimitive::PhysicsPrimitive() : m_Type(kUnknownShape), m_Kind(kStaticObject), m_Transform(kIdentity)
{
}

void PhysicsPrimitive::Create( const PhysicsPrimitiveInfo& Info )
{
    m_Type = Info.Type;
    m_Kind = Info.Mass > 0.f ? kDynamicObject : kStaticObject;

    auto Body = std::make_shared<BaseRigidBody>();
    Body->SetObjectType( m_Kind );
    Body->SetShapeType( m_Type );
    Body->SetMass( Info.Mass );
    Body->SetPosition( Convert(Info.Position) );
    Body->SetSize( Convert(Info.Size) );
    Body->Build();
    Body->JoinWorld( g_DynamicsWorld );

    m_Body.swap( Body );
}

void PhysicsPrimitive::Destroy()
{
    m_Body->LeaveWorld( g_DynamicsWorld );
}

void PhysicsPrimitive::Draw(const Math::Frustum& CameraFrustum)
{
    PrimitiveBatch::Append(
        m_Type,
        m_Transform,
        Convert(m_Body->GetSize()),
        CameraFrustum);
}

void PhysicsPrimitive::Update()
{
    m_Transform = Convert(m_Body->GetTransfrom());
}
