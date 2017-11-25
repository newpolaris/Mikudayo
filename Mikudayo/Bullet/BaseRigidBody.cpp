#include "stdafx.h"
#include "Physics.h"
#include "BaseRigidBody.h"
#include "Bullet/LinearMath.h"

using namespace Physics;

BaseRigidBody::DefaultMotionState::DefaultMotionState( const btTransform& startTransform, BaseRigidBody* parent )
    : m_parentRigidBodyRef(parent),
      m_startTransform(startTransform),
      m_worldTransform(startTransform)
{
}

BaseRigidBody::DefaultMotionState::~DefaultMotionState()
{
}

void BaseRigidBody::DefaultMotionState::getWorldTransform( btTransform& worldTransform ) const
{
    worldTransform = m_worldTransform;
}

void BaseRigidBody::DefaultMotionState::setWorldTransform( const btTransform& worldTransform )
{
    m_worldTransform = worldTransform;
}

BaseRigidBody::KinematicMotionState::KinematicMotionState(const btTransform &startTransform, BaseRigidBody *parent)
    : DefaultMotionState(startTransform, parent)
{
}

BaseRigidBody::KinematicMotionState::~KinematicMotionState()
{
}

void BaseRigidBody::KinematicMotionState::getWorldTransform(btTransform &worldTransform) const
{
    if (const BoneRef *boneRef = m_parentRigidBodyRef->boneRef()) {
        AffineTransform transform = boneRef->GetTransform();
        worldTransform = Convert(transform) * m_startTransform;
    }
    else {
        worldTransform.setIdentity();
    }
}

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
        return std::make_shared<btSphereShape>( m_Size.x() );
    case kBoxShape:
        return std::make_shared<btBoxShape>( m_Size );
    case kCapsuleShape:
        return std::make_shared<btCapsuleShape>( m_Size.x(), m_Size.y() );
    case kConeShape:
        return std::make_shared<btConeShape>( m_Size.x(), m_Size.y() );
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

//
// The rigidbody is centered on the zero point.
// 'm_Trans' has a difference to the bone's relative or zero point,
// depending on the presence or absence of a bone.
// If the model has a transform in this process, it will have an incorrect value.
// Also, even if we give the local coordinates to the process,
// When we create btRigidBody, it refer the transform through MotionState.
// This will introduce an incorrect relative distance between joints.
//
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
    Vector3 BonePosition( kZero );
	// If reference bone is exist, save a relative transform
    if (m_BoneRef.m_Instance != nullptr && m_BoneRef.m_Index >= 0)
        BonePosition = m_BoneRef.GetTransform().GetTranslation();
    m_Trans = btTransform( m_Rotation, m_Position - Convert(BonePosition) );
    m_InvTrans = m_Trans.inverse();

    btTransform worldTransform( m_Rotation, m_Position );

    if (m_Type == kStaticObject && m_BoneRef.m_Instance)
        m_MotionState = std::make_shared<KinematicMotionState>( m_Trans, this );
    else
        m_MotionState = std::make_shared<DefaultMotionState>( worldTransform, this );

    btRigidBody::btRigidBodyConstructionInfo info(massValue, m_MotionState.get(), Shape, localInertia);
    info.m_linearDamping = m_linearDamping;
    info.m_angularDamping = m_angularDamping;
    info.m_restitution = m_Restitution;
    info.m_friction = m_friction;
    // additional damping can help avoiding lowpass jitter motion, help stability for ragdolls etc.
    info.m_additionalDamping = true;
    std::shared_ptr<btRigidBody> body = std::make_shared<btRigidBody>( info );
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

void BaseRigidBody::SyncLocalTransform()
{
    if (m_BoneRef.m_Index < 0) 
        return;
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

    if (m_Type != kStaticObject && m_BoneRef.m_Instance != nullptr)
    {
        btTransform centerOfMassTransform = m_Body->getCenterOfMassTransform();
        btTransform tr = centerOfMassTransform * m_InvTrans;
        //
        // Remove the disparity from bone to bone connection.
        // Even joint has 0 linear limit, small linear movement would be happend.
        // Align the bone position to the non-simulated position.
        //
        if (m_Type == kAlignedObject)
        {
            m_BoneRef.UpdateLocalTransform();
            tr.setOrigin( Convert(m_BoneRef.GetTransform().GetTranslation()) );
            // Update rigid-body
            m_Body->setCenterOfMassTransform( tr * m_Trans );
        }
        m_BoneRef.SetTransform( tr );
    }
}

void BaseRigidBody::JoinWorld( btDynamicsWorld* world )
{
    // When use bullet 2.75, casting is needed
    world->addRigidBody( m_Body.get(), m_GroupID, m_CollisionGroupMask );
    m_Body->setUserPointer( this );
}

void BaseRigidBody::LeaveWorld( btDynamicsWorld* world )
{
    // When use bullet 2.75, casting is needed
    world->removeRigidBody( m_Body.get() );
    m_Body->setUserPointer( nullptr );
}

void BaseRigidBody::UpdateTransform( btDynamicsWorld* world )
{
    // http://www.bulletphysics.org/Bullet/phpBB3/viewtopic.php?f=9&t=2513
     
    AffineTransform transform = (m_BoneRef.m_Index >= 0) ? m_BoneRef.GetTransform() : AffineTransform(kIdentity);
    const btTransform newTransform = Convert(transform) * m_Trans;
    m_MotionState->setWorldTransform(newTransform);
    m_Body->setInterpolationWorldTransform( newTransform );
    m_Body->setWorldTransform( newTransform );
    m_Body->setActivationState( DISABLE_DEACTIVATION );
    m_Body->activate();
    world->getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs( m_Body->getBroadphaseHandle(), world->getDispatcher() );
    m_Body->setLinearVelocity( btVector3( 0, 0, 0 ) );
    m_Body->setAngularVelocity( btVector3( 0, 0, 0 ) );
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

BoneRef* BaseRigidBody::boneRef()
{
    if (m_BoneRef.m_Instance == nullptr)
        return nullptr;
    return &m_BoneRef;
}
