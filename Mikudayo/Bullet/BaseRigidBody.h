//
// Used MMDAI's RigidBody code
//
// Original Boilerplate:
//
/**

 Copyright (c) 2010-2014  hkrn

 All rights reserved.

 Redistribution and use in source and binary forms, with or
 without modification, are permitted provided that the following
 conditions are met:

 - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 - Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
 - Neither the name of the MMDAI project team nor the names of
   its contributors may be used to endorse or promote products
   derived from this software without specific prior written
   permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include <memory>

#include "IRigidBody.h"
#define BT_NO_SIMD_OPERATOR_OVERLOADS 1
#include "LinearMath/btVector3.h"
#include "LinearMath/btTransform.h"

#include "PmxInstant.h"

class btRigidBody;
class btCollisionShape;
class btMotionState;
class btDynamicsWorld;

namespace Math
{
    class Vector3;
    class Quaternion;
}

class BaseRigidBody
{
public:
    BaseRigidBody();
    BaseRigidBody( const BaseRigidBody& ) = delete;
    BaseRigidBody& operator=( const BaseRigidBody& ) = delete;
    virtual ~BaseRigidBody() {}

    void Build();

    std::shared_ptr<btCollisionShape> CreateShape() const;
    std::shared_ptr<btRigidBody> CreateRigidBody( btCollisionShape* shape );
    const btTransform CreateTransform() const;

    ObjectType GetType() const;
    ShapeType GetShapeType() const;
    btRigidBody* GetBody() const;
    btTransform GetTransfrom() const;
    btVector3 GetSize() const;

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

    void syncLocalTransform();
    void JoinWorld( btDynamicsWorld* world );
    void LeaveWorld( btDynamicsWorld* world );
    void UpdateTransform();

protected:
    ObjectType m_Type;
    ShapeType m_ShapeType;
    btVector3 m_Size;
    btVector3 m_Position;
    btQuaternion m_Rotation;
    btTransform m_WorldTransform;
    btTransform m_World2LocalTransform;

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