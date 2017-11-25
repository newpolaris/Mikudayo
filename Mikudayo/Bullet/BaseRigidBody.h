//
// Used MMDAgent's PMDRigidBody code
// Copyright (c) 2009-2016  Nagoya Institute of Technology Department of Computer Science
//
// Used MMDAI's RigidBody code
// Copyright (c) 2010-2014  hkrn
// 
#pragma once

#include <memory>

#include "IRigidBody.h"
#define BT_NO_SIMD_OPERATOR_OVERLOADS 1
#include "LinearMath/btVector3.h"
#include "LinearMath/btTransform.h"
#include "LinearMath/btMotionState.h"

#include "PmxInstant.h"

class btRigidBody;
class btCollisionShape;
class btMotionState;
class btDynamicsWorld;
class BoneRef;

namespace Math
{
    class Vector3;
    class Quaternion;
}

class BaseRigidBody
{
public:

    class DefaultMotionState : public btMotionState {
    public:

        DefaultMotionState( const btTransform& startTransform, BaseRigidBody* parent );
        ~DefaultMotionState();

        void getWorldTransform( btTransform& worldTransform ) const override;
        void setWorldTransform( const btTransform& worldTransform ) override;

    protected:
        BaseRigidBody *m_parentRigidBodyRef;
        const btTransform m_startTransform;

        btTransform m_worldTransform;
    };

    class KinematicMotionState : public DefaultMotionState {
    public:
        KinematicMotionState(const btTransform& startTransform, BaseRigidBody* parent);
        ~KinematicMotionState();

        void getWorldTransform(btTransform& worldTransform) const;
    };

    BaseRigidBody();
    BaseRigidBody( const BaseRigidBody& ) = delete;
    BaseRigidBody& operator=( const BaseRigidBody& ) = delete;
    virtual ~BaseRigidBody() {}

    void Build();

    std::shared_ptr<btCollisionShape> CreateShape() const;
    std::shared_ptr<btRigidBody> CreateRigidBody( btCollisionShape* shape );

    ObjectType GetType() const;
    ShapeType GetShapeType() const;
    btRigidBody* GetBody() const;
    btTransform GetTransfrom() const;
    btVector3 GetSize() const;

    BoneRef *boneRef();

    void SetAngularDamping( float value );
    void SetBoneRef( BoneRef boneRef );
    void SetCollisionGroupID( uint8_t value );
    void SetCollisionMask( uint16_t value );
    void SetFriction( float value );
    void SetLinearDamping( float value );
    void SetMass( float Mass );
    void SetObjectType( ObjectType Type );
    void SetPosition( const Math::Vector3& value );
    void SetRestitution( float value );
    void SetRotation( const Math::Quaternion& value );
    void SetShapeType( ShapeType Type );
    void SetSize( const Math::Vector3& value );

    void SyncLocalTransform();
    void JoinWorld( btDynamicsWorld* world );
    void LeaveWorld( btDynamicsWorld* world );
    void UpdateTransform( btDynamicsWorld* world );

protected:
    ObjectType m_Type;
    ShapeType m_ShapeType;
    btVector3 m_Size;
    btVector3 m_Position;
    btQuaternion m_Rotation;
    btTransform m_Trans;
    btTransform m_InvTrans;

    float m_Mass;
    float m_linearDamping;
    float m_angularDamping;
    float m_Restitution;
    float m_friction;

    uint16_t m_GroupID;
    uint16_t m_CollisionGroupMask;
    uint8_t m_CollisionGroupID;

    BoneRef m_BoneRef;
    std::shared_ptr<btRigidBody> m_Body;
    std::shared_ptr<btCollisionShape> m_Shape;
    std::shared_ptr<btMotionState> m_MotionState;
};

inline ObjectType BaseRigidBody::GetType() const
{
    return m_Type;
}

inline ShapeType BaseRigidBody::GetShapeType() const
{
    return m_ShapeType;
}

inline btRigidBody* BaseRigidBody::GetBody() const
{
    return m_Body.get();
}
inline btVector3 BaseRigidBody::GetSize() const
{
    return m_Size;
}

