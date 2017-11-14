#include "stdafx.h"
#include "BaseJoint.h"
#include "Physics.h"
#include "RigidBody.h"
#include "Bullet/LinearMath.h"
#include "BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.h"
#include "BulletDynamics/ConstraintSolver/btGeneric6DofSpring2Constraint.h"
#include "BulletDynamics/ConstraintSolver/btConeTwistConstraint.h"

using namespace Math;

namespace {
    const btScalar kDefaultDamping = 0.25f;
    const btScalar kDebugDrawSize = 0.50f;
}

BaseJoint::BaseJoint(bool bRH) : 
    m_bRightHand( bRH ),
    m_Type( JointType::kGeneric6DofSpring ),
    m_Position( 0, 0, 0 ),
    m_Rotation( 0, 0, 0, 1 ),
    m_LinearLowerLimit( 0, 0, 0 ),
    m_LinearUpperLimit( 0, 0, 0 ),
    m_AngularLowerLimit( 0, 0, 0 ),
    m_AngularUpperLimit( 0, 0, 0 ),
    m_LinearStiffness( 0, 0, 0 ),
    m_AngularStiffness( 0, 0, 0 )
{
}

btTypedConstraintPtr BaseJoint::CreateConstraint()
{
    btTransform worldTransform;
	worldTransform.setIdentity();
    worldTransform.setRotation( m_Rotation );
    worldTransform.setOrigin( m_Position );

    btTransform invTransformA = m_RigidBodyA->GetBody()->getCenterOfMassTransform().inverse();
    btTransform invTransformB = m_RigidBodyB->GetBody()->getCenterOfMassTransform().inverse();
    btTransform frameInA = invTransformA * worldTransform;
    btTransform frameInB = invTransformB * worldTransform;

    btRigidBody *pBodyA = m_RigidBodyA->GetBody(), *pBodyB = m_RigidBodyB->GetBody();

    if (m_bRightHand)
    {
        auto lower = m_LinearLowerLimit, upper = m_LinearUpperLimit;
        m_LinearLowerLimit = btVector3( lower.x(), lower.y(), -upper.z() );
        m_LinearUpperLimit = btVector3( upper.x(), upper.y(), -lower.z() );
    }
    if (m_bRightHand)
    {
        auto lower = m_AngularLowerLimit, upper = m_AngularUpperLimit;
        m_AngularLowerLimit = btVector3( -upper.x(), -upper.y(), lower.z() );
        m_AngularUpperLimit = btVector3( -lower.x(), -lower.y(), upper.z() );
    }

    switch (m_Type) {
    case JointType::kGeneric6Dof:
    {
        auto pConstraint = std::make_shared<btGeneric6DofConstraint>( *pBodyA, *pBodyB, frameInA, frameInB, true );
        pConstraint->setLinearUpperLimit( m_LinearLowerLimit );
        pConstraint->setLinearLowerLimit( m_LinearUpperLimit );
        pConstraint->setAngularLowerLimit( m_AngularLowerLimit );
        pConstraint->setAngularUpperLimit( m_AngularUpperLimit );

        return pConstraint;
    }
    case JointType::kGeneric6DofSpring:
    {
        auto pConstraint = std::make_shared<btGeneric6DofSpringConstraint>( *pBodyA, *pBodyB, frameInA, frameInB, true );
        pConstraint->setLinearUpperLimit( m_LinearLowerLimit );
        pConstraint->setLinearLowerLimit( m_LinearUpperLimit );
        pConstraint->setAngularLowerLimit( m_AngularLowerLimit );
        pConstraint->setAngularUpperLimit( m_AngularUpperLimit );

        //
        // Used MMDAI's RigidBody code
        // Copyright (c) 2010-2014  hkrn
        //
        for (int i = 0; i < 3; i++)
        {
            const btScalar& value = m_LinearStiffness[i];
            if (!btFuzzyZero( value ))
            {
                pConstraint->enableSpring( i, true );
                pConstraint->setStiffness( i, value );
                pConstraint->setDamping( i, kDefaultDamping );
            }
        }

        for (int i = 0; i < 3; i++)
        {
            const btScalar& value = m_AngularStiffness[i];
            if (!btFuzzyZero( value ))
            {
                int index = i + 3;
                pConstraint->enableSpring( index, true );
                pConstraint->setStiffness( index, value );
                pConstraint->setDamping( index, kDefaultDamping );
            }
        }
        return pConstraint;
    }
    case JointType::kPoint2Point:
    {
        return std::make_shared<btPoint2PointConstraint>( *pBodyA, *pBodyB, btVector3(0,0,0), btVector3(0,0,0) );
    }
    case JointType::kConeTwist:
    {
        auto pConstraint = std::make_shared<btConeTwistConstraint>( *pBodyA, *pBodyB, frameInA, frameInB );

        //
        // Used MMDAI's RigidBody code
        // Copyright (c) 2010-2014  hkrn
        //
        // (_swingSpan1, _swingSpan2, _twistSpan, _softness, _biasFactor, _relaxationFactor)
        pConstraint->setLimit( m_AngularLowerLimit.x(), m_AngularLowerLimit.y(), m_AngularLowerLimit.z(),
            m_LinearStiffness.x(), m_LinearStiffness.y(), m_LinearStiffness.z() );
        pConstraint->setDamping( m_LinearLowerLimit.x() );
        pConstraint->setFixThresh( m_LinearLowerLimit.x() );
        bool enableMotor = btFuzzyZero( m_LinearLowerLimit.z() );
        pConstraint->enableMotor( enableMotor );
        if (enableMotor)
        {
            pConstraint->setMaxMotorImpulse( m_LinearUpperLimit.z() );
            btQuaternion rotation;
            rotation.setEuler( m_AngularStiffness.x(), m_AngularStiffness.y(), m_AngularStiffness.z() );
            pConstraint->setMotorTargetInConstraintSpace( rotation );
        }
        return pConstraint;
    }
    case JointType::kSlider:
    {
        auto pConstraint = std::make_shared<btSliderConstraint>( *pBodyA, *pBodyB, frameInA, frameInB, true );
        pConstraint->setLowerLinLimit( m_LinearLowerLimit.x() );
        pConstraint->setUpperLinLimit( m_LinearUpperLimit.x() );
        pConstraint->setLowerAngLimit( m_AngularLowerLimit.x() );
        pConstraint->setUpperAngLimit( m_AngularUpperLimit.x() );

        //
        // Used MMDAI's RigidBody code
        // Copyright (c) 2010-2014  hkrn
        //
        bool enablePoweredLinMotor = btFuzzyZero( m_LinearStiffness.x() );
        pConstraint->setPoweredLinMotor( enablePoweredLinMotor );
        if (enablePoweredLinMotor) {
            pConstraint->setTargetLinMotorVelocity( m_LinearStiffness.y() );
            pConstraint->setMaxLinMotorForce( m_LinearStiffness.z() );
        }
        bool enablePoweredAngMotor = btFuzzyZero( m_AngularStiffness.x() );
        pConstraint->setPoweredAngMotor( enablePoweredAngMotor );
        if (enablePoweredAngMotor) {
            pConstraint->setTargetAngMotorVelocity( m_AngularStiffness.y() );
            pConstraint->setMaxAngMotorForce( m_AngularStiffness.z() );
        }
        return pConstraint;
    }
    case JointType::kHinge:
    {
        auto pConstraint = std::make_shared<btHingeConstraint>( *pBodyA, *pBodyB, frameInA, frameInB, true );

        //
        // Used MMDAI's RigidBody code
        // Copyright (c) 2010-2014  hkrn
        //
        pConstraint->setLimit( m_AngularLowerLimit.x(), m_AngularUpperLimit.y(), m_LinearStiffness.x(),
            m_LinearStiffness.y(), m_LinearStiffness.z() );
        bool enableMotor = btFuzzyZero( m_AngularStiffness.z() );
        pConstraint->enableMotor( enableMotor );
        if (enableMotor) {
            pConstraint->enableAngularMotor( enableMotor, m_AngularStiffness.y(), m_AngularStiffness.z() );
        }
        return pConstraint;
    }
    default:
        return nullptr;
    }
}

void BaseJoint::Build()
{
    m_Constraint = CreateConstraint();
    m_Constraint->setDbgDrawSize( kDebugDrawSize );
    if (m_Constraint)
    {
        m_RigidBodyA->GetBody()->addConstraintRef( m_Constraint.get() );
        m_RigidBodyB->GetBody()->addConstraintRef( m_Constraint.get() );
    }
}

void BaseJoint::JoinWorld( btDynamicsWorld* world )
{
    world->addConstraint( m_Constraint.get() );
}

void BaseJoint::LeaveWorld( btDynamicsWorld* world )
{
    world->removeConstraint( m_Constraint.get() );
}

void BaseJoint::SetType( const JointType type )
{
    m_Type = type;
}

void BaseJoint::SetRigidBodyA( RigidBodyPtr& rigidBody )
{
    m_RigidBodyA = rigidBody;
}

void BaseJoint::SetRigidBodyB( RigidBodyPtr& rigidBody )
{
    m_RigidBodyB = rigidBody;
}

void BaseJoint::SetPosition( const Math::Vector3& position )
{
    m_Position = Convert( position );
}

void BaseJoint::SetRotation( const Math::Quaternion& rotation )
{
    m_Rotation = Convert( rotation );
}

void BaseJoint::SetLinearLowerLimit( const Math::Vector3& limit )
{
    m_LinearLowerLimit = Convert( limit );
}

void BaseJoint::SetLinearUpperLimit( const Math::Vector3& limit )
{
    m_LinearUpperLimit = Convert( limit );
}

void BaseJoint::SetAngularLowerLimit( const Math::Vector3& limit )
{
    m_AngularLowerLimit = Convert( limit );
}

void BaseJoint::SetAngularUpperLimit( const Math::Vector3& limit )
{
    m_AngularUpperLimit = Convert( limit );
}

void BaseJoint::SetLinearStiffness( const Math::Vector3& limit )
{
    m_LinearStiffness = Convert( limit );
}

void BaseJoint::SetAngularStiffness( const Math::Vector3& limit )
{
    m_AngularStiffness = Convert( limit );
}
