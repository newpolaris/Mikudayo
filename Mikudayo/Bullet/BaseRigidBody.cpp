#include "stdafx.h"
#include "Physics.h"
#include "BaseRigidBody.h"

using namespace Physics;

BaseRigidBody::BaseRigidBody() :
    m_angularDamping( 0 ),
    m_friction( 0.5f ),
    m_linearDamping( 0 ),
    m_mass( 0 ),
    m_Position( 0, 0, 0 ),
    m_Rotation( 0, 0, 0, 1 ),
    m_Restitution( 0 ),
    m_Size( 0, 0, 0 ),
    m_groupID( 0 ),
    m_collisionGroupMask( 0 ),
    m_collisionGroupID( 0 ),
    m_Type( kStaticObject ),
    m_ShapeType( kUnknownShape )
{
}

std::shared_ptr<btCollisionShape> BaseRigidBody::CreateShape() const
{
    switch (m_ShapeType) {
    case kSphereShape:
        return std::make_shared<btSphereShape>(m_Size.x());
    case kBoxShape:
        return std::make_shared<btBoxShape>(m_Size);
    case kCapsuleShape:
        return std::make_shared<btCapsuleShape>(m_Size.x(), m_Size.y());
    case kConeShape:
        return std::make_shared<btConeShape>(m_Size.x(), m_Size.y());
    case kCylinderShape:
        return std::make_shared<btCylinderShape>( m_Size );
    case kPlaneShape:
        return std::make_shared<btStaticPlaneShape>( btVector3(0, 1, 0), 1.f );
    case kUnknownShape:
    default:
        ASSERT(false);
        return nullptr;
    }
}

std::shared_ptr<btRigidBody> BaseRigidBody::CreateRigidBody( btCollisionShape* Shape )
{
    btVector3 localInertia(0, 0, 0);
    btScalar massValue(0);
    if (m_Type != kStaticObject) {
        massValue = m_mass;
        if (Shape && !btFuzzyZero(massValue)) {
            Shape->calculateLocalInertia(massValue, localInertia);
        }
    }
    m_worldTransform = CreateTransform();
    m_MotionState = std::make_shared<btDefaultMotionState>( m_worldTransform );

    btRigidBody::btRigidBodyConstructionInfo info(massValue, m_MotionState.get(), Shape, localInertia);
    info.m_linearDamping = m_linearDamping;
    info.m_angularDamping = m_angularDamping;
    info.m_restitution = m_Restitution;
    info.m_friction = m_friction;
    std::shared_ptr<btRigidBody> body = std::make_shared<btRigidBody>( info );

    return body;
}

const btTransform BaseRigidBody::CreateTransform() const
{
    return btTransform( m_Rotation, m_Position );
}

void BaseRigidBody::Build()
{
    m_Shape = CreateShape();
    m_Body = CreateRigidBody( m_Shape.get() );
}

btTransform BaseRigidBody::GetTransfrom() const
{
    btTransform transform;
    m_MotionState->getWorldTransform(transform);
    return transform;
}

void BaseRigidBody::JoinWorld( void* value )
{
    auto DynamicsWorld = reinterpret_cast<btDynamicsWorld*>( value );
    DynamicsWorld->addRigidBody( m_Body.get() );
}

void BaseRigidBody::LeaveWorld( void* value )
{
    auto DynamicsWorld = reinterpret_cast<btDynamicsWorld*>( value );
    DynamicsWorld->removeRigidBody( m_Body.get() );
}

void BaseRigidBody::SetAngularDamping( float value )
{
    m_angularDamping = value;
}

void BaseRigidBody::SetCollisionGroupID( uint8_t value )
{
    m_collisionGroupID = value;
}

void BaseRigidBody::SetCollisionMask( uint16_t value )
{
    m_collisionGroupMask = value;
}

void BaseRigidBody::SetFriction( float value )
{
    m_friction = value;
}

void BaseRigidBody::SetLinearDamping( float value )
{
    m_linearDamping = value;
}

void BaseRigidBody::SetMass( float value )
{
    m_mass = value;
}

void BaseRigidBody::SetObjectType( ObjectType Type )
{
    m_Type = Type;
}

void BaseRigidBody::SetPosition( const btVector3& value )
{
    m_Position = value;

}

void BaseRigidBody::SetRestitution( float value )
{
    m_Restitution = value;
}

void BaseRigidBody::SetRotation( const btQuaternion& value )
{
    m_Rotation = value;
}

void BaseRigidBody::SetShapeType( ShapeType Type )
{
    m_ShapeType = Type;
}

void BaseRigidBody::SetSize( const btVector3& value )
{
    m_Size = value;
}