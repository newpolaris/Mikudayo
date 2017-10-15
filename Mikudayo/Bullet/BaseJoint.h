#pragma once

#include <memory>

#define BT_NO_SIMD_OPERATOR_OVERLOADS 1
#include "LinearMath/btVector3.h"
#include "LinearMath/btTransform.h"

class btRigidBody;
class btCollisionShape;
class btMotionState;
class btDynamicsWorld;

using RigidBodyPtr = std::shared_ptr<class RigidBody>;

namespace Math
{
    class Vector3;
    class Quaternion;
}
#pragma once

enum class JointType : uint8_t
{
    kGeneric6DofSpring = 0,
    kGeneric6Dof = 1,
    kPoint2Point = 2,
    kConeTwist = 3,
    kSlider = 5,
    kHinge = 6
};

using btTypedConstraintPtr = std::shared_ptr<class btTypedConstraint>;
class BaseJoint
{
public:

    BaseJoint(bool bRH = true);

    btTypedConstraintPtr CreateConstraint();

    void Build();

    void JoinWorld( btDynamicsWorld* world );
    void LeaveWorld( btDynamicsWorld* world );

    void SetType( const JointType type );
    void SetRigidBodyA( RigidBodyPtr& rigidBody );
    void SetRigidBodyB( RigidBodyPtr& rigidBody );
    void SetPosition( const Math::Vector3& position );
    void SetRotation( const Math::Quaternion& rotation );
    void SetLinearLowerLimit( const Math::Vector3& limit );
    void SetLinearUpperLimit( const Math::Vector3& limit );
    void SetAngularLowerLimit( const Math::Vector3& limit );
    void SetAngularUpperLimit( const Math::Vector3& limit );
    void SetLinearStiffness( const Math::Vector3& limit );
    void SetAngularStiffness( const Math::Vector3& limit );

protected:

    bool m_bRightHand;
    JointType m_Type;
    btVector3 m_Position;
    btQuaternion m_Rotation;
    btVector3 m_LinearLowerLimit; // limits move
    btVector3 m_LinearUpperLimit;
    btVector3 m_AngularLowerLimit; // limits rot
    btVector3 m_AngularUpperLimit;
    btVector3 m_LinearStiffness; // spring move coefficient
    btVector3 m_AngularStiffness; // spring rotation coefficient

    RigidBodyPtr m_RigidBodyA;
    RigidBodyPtr m_RigidBodyB;
    std::shared_ptr<btTypedConstraint> m_Constraint;
};
