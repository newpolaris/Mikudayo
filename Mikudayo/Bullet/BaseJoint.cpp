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
}

BaseJoint::BaseJoint() : 
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
    worldTransform.setRotation( m_Rotation );
    worldTransform.setOrigin( m_Position );

    switch (m_Type) {
    case JointType::kGeneric6Dof:
    {
    #if 0
        auto pGen6Dof = std::make_shared<btGeneric6DofConstraint>( btRigidBody& rbA, btRigidBody& rbB, const btTransform& frameInA, const btTransform& frameInB, true );
        pGen6Dof->setDbgDrawSize( btScalar( 5.f ) );
        pGen6Dof->setAngularLowerLimit( btVector3( 0, 0, 0 ) );
        pGen6Dof->setAngularUpperLimit( btVector3( 0, 0, 0 ) );
        pGen6Dof->setLinearLowerLimit( btVector3( -10., 0, 0 ) );
        pGen6Dof->setLinearUpperLimit( btVector3( 10., 0, 0 ) );

        pGen6Dof->getTranslationalLimitMotor()->m_enableMotor[0] = true;
        pGen6Dof->getTranslationalLimitMotor()->m_targetVelocity[0] = 5.0f;
        pGen6Dof->getTranslationalLimitMotor()->m_maxMotorForce[0] = 0.1f;

        return pGen6Dof;
    #endif
    }
    case JointType::kGeneric6DofSpring:
    {
        btTransform invTransformA = m_RigidBodyA->GetBody()->getCenterOfMassTransform().inverse();
        btTransform invTransformB = m_RigidBodyB->GetBody()->getCenterOfMassTransform().inverse();
        btTransform frameInA = invTransformA * worldTransform;
        btTransform frameInB = invTransformB * worldTransform;

        auto pBodyA = m_RigidBodyA->GetBody(), pBodyB = m_RigidBodyB->GetBody();
		auto pConstraint = std::make_shared<btGeneric6DofSpringConstraint>(*pBodyA, *pBodyB, frameInA, frameInB, true);
        pConstraint->setDbgDrawSize( btScalar( 5.f ) );
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
            if (!btFuzzyZero(value)) 
            {
                pConstraint->enableSpring(i, true);
                pConstraint->setStiffness(i, value);
                pConstraint->setDamping(i, kDefaultDamping);
            }
        }

        for (int i = 0; i < 3; i++) 
        {
            const btScalar& value = m_AngularStiffness[i];
            if (!btFuzzyZero(value)) 
            {
                int index = i + 3;
                pConstraint->enableSpring(index, true);
                pConstraint->setStiffness(index, value);
                pConstraint->setDamping(index, kDefaultDamping);
            }
        }
        return pConstraint;
    }
    case JointType::kPoint2Point:
    {
        // btTypedConstraint* p2p = new btPoint2PointConstraint( *body0, pivotInA );
        // btTypedConstraint* p2p = new btPoint2PointConstraint(*body0,*body1,pivotInA,pivotInB);
    }
    case JointType::kConeTwist:
    {
        /*
		btTransform frameInA, frameInB;
		frameInA = btTransform::getIdentity();
		frameInA.getBasis().setEulerZYX(0, 0, SIMD_PI_2);
		frameInA.setOrigin(btVector3(btScalar(0.), btScalar(-5.), btScalar(0.)));
		frameInB = btTransform::getIdentity();
		frameInB.getBasis().setEulerZYX(0,0,  SIMD_PI_2);
		frameInB.setOrigin(btVector3(btScalar(0.), btScalar(5.), btScalar(0.)));

        auto joint = std::make_shared<btConeTwistConstraint>();// *pBodyA, *pBodyB, frameInA, frameInB );
		m_ctc->setLimit(btScalar(SIMD_PI_4*0.6f), btScalar(SIMD_PI_4), btScalar(SIMD_PI) * 0.8f, 0.5f);
        joint->
        */
    }
    // case JointType::kSlider:
    // case JointType::kHinge:
		//btTypedConstraint* hinge = new btHingeConstraint(*body0,*body1,pivotInA,pivotInB,axisInA,axisInB);
    }
}

void BaseJoint::Build()
{
    m_Constraint = CreateConstraint();

    m_RigidBodyA->GetBody()->addConstraintRef( m_Constraint.get() );
    m_RigidBodyB->GetBody()->addConstraintRef( m_Constraint.get() );
}

void BaseJoint::JoinWorld( btDynamicsWorld* world )
{
    world->addConstraint( m_Constraint.get() );
    m_Constraint->setUserConstraintPtr( this );
}

void BaseJoint::LeaveWorld( btDynamicsWorld* world )
{
    world->removeConstraint( m_Constraint.get() );
    m_Constraint->setUserConstraintPtr( nullptr );
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
