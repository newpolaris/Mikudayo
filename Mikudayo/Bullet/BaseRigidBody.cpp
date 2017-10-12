#include "stdafx.h"
#include "Physics.h"
#include "BaseRigidBody.h"
#include "Bullet/LinearMath.h"

using namespace Physics;

BaseRigidBody::BaseRigidBody() :
    m_angularDamping( 0 ),
    m_friction( 0.5f ),
    m_linearDamping( 0 ),
    m_Mass( 0 ),
    m_Position( 0, 0, 0 ),
    m_Rotation( 0, 0, 0, 1 ),
    m_Restitution( 0 ),
    m_Size( 0, 0, 0 ),
    m_GroupID( 0 ),
    m_CollisionGroupMask( 0 ),
    m_CollisionGroupID( 0 ),
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
        massValue = m_Mass;
        if (Shape && !btFuzzyZero(massValue)) {
            Shape->calculateLocalInertia(massValue, localInertia);
        }
    }
    m_WorldTransform = CreateTransform();
    m_World2LocalTransform = m_WorldTransform.inverse();
    m_MotionState = std::make_shared<btDefaultMotionState>( m_WorldTransform );

    btRigidBody::btRigidBodyConstructionInfo info(massValue, m_MotionState.get(), Shape, localInertia);
    info.m_linearDamping = m_linearDamping;
    info.m_angularDamping = m_angularDamping;
    info.m_restitution = m_Restitution;
    info.m_friction = m_friction;
    info.m_additionalDamping = true;
    std::shared_ptr<btRigidBody> body = std::make_shared<btRigidBody>( info );
    body->setActivationState( DISABLE_DEACTIVATION );
    body->setUserPointer( this );
    switch (m_Type) {
    case kStaticObject:
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        break;
    default:
        break;
    }
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

void BaseRigidBody::syncLocalTransform()
{
    if (m_Type != kStaticObject)
    {
        AffineTransform centerOfMassTransform = Convert( m_Body->getCenterOfMassTransform() );
        const AffineTransform& worldBoneTransform = AffineTransform(m_BoneRef.GetLocalTransform()) * Convert( m_WorldTransform );
        if (m_Type == kAlignedObject) {
            centerOfMassTransform.SetTranslation( worldBoneTransform.GetTranslation() );
            m_Body->setCenterOfMassTransform( Convert( centerOfMassTransform ) );
        }
    #if 0
        const int nconstraints = m_body->getNumConstraintRefs();
        for (int i = 0; i < nconstraints; i++) {
            btTypedConstraint *constraint = m_body->getConstraintRef( i );
            if (constraint->getConstraintType() == D6_CONSTRAINT_TYPE) {
                btGeneric6DofConstraint *dof = static_cast<btGeneric6DofSpringConstraint *>(constraint);
                btTranslationalLimitMotor *motor = dof->getTranslationalLimitMotor();
                if (motor->m_lowerLimit.isZero() && motor->m_upperLimit.isZero()) {
                }
            }
        }
    #endif
        const AffineTransform& localTransform = centerOfMassTransform * Convert(m_World2LocalTransform);
        const OrthogonalTransform localOrth( Quaternion( localTransform.GetBasis() ), localTransform.GetTranslation() );
        m_BoneRef.SetLocalTransform( localOrth );
    }
}

void BaseRigidBody::JoinWorld( btDynamicsWorld* world )
{
#if 1
    world->addRigidBody( m_Body.get(), m_GroupID, m_CollisionGroupMask );
#else
    world->addRigidBody( m_Body.get() );
#endif
    m_Body->setUserPointer( this );
}

void BaseRigidBody::LeaveWorld( btDynamicsWorld* world )
{
    world->removeRigidBody( m_Body.get() );
    m_Body->setUserPointer( nullptr );
}

void BaseRigidBody::UpdateTransform()
{
    const btTransform& newTransform = Convert(m_BoneRef.GetLocalTransform()) * m_WorldTransform;
    m_MotionState->setWorldTransform( newTransform );
    m_Body->setInterpolationWorldTransform( newTransform );
}

void BaseRigidBody::SetAngularDamping( float value )
{
    m_angularDamping = value;
}

void BaseRigidBody::SetBoneRef( BoneRef boneRef )
{
    m_BoneRef = boneRef;
}

void BaseRigidBody::SetCollisionGroupID( uint8_t value )
{
    m_CollisionGroupID = value;
    m_GroupID = uint16_t( 0x0001 << m_CollisionGroupID );
}

void BaseRigidBody::SetCollisionMask( uint16_t value )
{
    m_CollisionGroupMask = value;
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
    m_Mass = value;
}

void BaseRigidBody::SetObjectType( ObjectType Type )
{
    m_Type = Type;
}

void BaseRigidBody::SetPosition( const Vector3& value )
{
    m_Position = Convert(value);
}

void BaseRigidBody::SetRestitution( float value )
{
    m_Restitution = value;
}

void BaseRigidBody::SetRotation( const Quaternion& value )
{
    m_Rotation = Convert(value);
}

void BaseRigidBody::SetShapeType( ShapeType Type )
{
    m_ShapeType = Type;
}

void BaseRigidBody::SetSize( const Vector3& value )
{
    m_Size = Convert( value );
}