#include "stdafx.h"
#include "PmxModel.h"
#include "PmxInstant.h"
#include "Vmd.h"
#include "KeyFrameAnimation.h"
#include "PrimitiveUtility.h"
#include "Visitor.h"
#include "Bullet/Physics.h"
#include "Bullet/RigidBody.h"
#include "Bullet/Joint.h"
#include "Bullet/LinearMath.h"

using namespace Utility;
using namespace Math;
using namespace Graphics;
using namespace Physics;

namespace {
	enum ETextureType
	{
		kTextureDiffuse,
		kTextureSphere,
		kTextureToon,
		kTextureMax
	};

	struct VertexProperty
	{
		XMFLOAT3 Normal;
		XMFLOAT2 UV;
        uint32_t BoneID[4] = {0, };
        float    Weight[4] = {0.f };
		float    EdgeSize;
	};

    template <typename T>
    size_t GetVectorSize( const std::vector<T>& vec )
    {
        return sizeof( T ) * vec.size();
    }
}

BoolVar s_bDrawBoundingSphere( "Application/Model/Draw Bounding Shphere", false );
// If model is mixed with sky box, model's boundary is exculde by 's_ExcludeRange'
BoolVar s_bExcludeSkyBox( "Application/Model/Exclude Sky Box", true );
NumVar s_ExcludeRange( "Application/Model/Exclude Range", 1000.f, 500.f, 10000.f );

struct PmxInstant::Context final
{
    Context( PmxModel& model, PmxInstant* parent );
    ~Context();
    void Clear( void );
    void Draw( GraphicsContext& gfxContext, Visitor& visitor );
    void DrawBone();
    bool LoadModel();
    bool LoadMotion( const std::wstring& FilePath );
    void JoinWorld( btDynamicsWorld* world );
    void LeaveWorld( btDynamicsWorld* world );
    void SetPosition( const Vector3& postion );
    void SetupSkeleton( const std::vector<PmxModel::Bone>& Bones );
    void Update( float kFrameTime );
    void UpdateAfterPhysics( float kFrameTime );

    const OrthogonalTransform GetTransform( uint32_t i ) const;
    void UpdateLocalTransform( uint32_t i );
    void SetLocalTransform( uint32_t i, const OrthogonalTransform& transform );

protected:

    void LoadBoneMotion( const std::vector<Vmd::BoneFrame>& frames );
    void PerformTransform( uint32_t i );
    void UpdateChildPose( int32_t idx );
    void UpdateIK( const PmxModel::IKAttr& ik );
    void UpdatePose();

    PmxModel& m_Model;
    PmxInstant* m_Parent;
    bool m_bRightHand;
    Matrix4 m_ModelTransform;

    // Skinning
    std::vector<Quaternion> localInherentOrientations;
    std::vector<Vector3> localInherentTranslations;
    std::vector<OrthogonalTransform> m_toRoot; // inverse inital pose ( inverse Rest)
    std::vector<OrthogonalTransform> m_LocalPose;
    std::vector<OrthogonalTransform> m_LocalPoseDefault; // offset matrix
    std::vector<OrthogonalTransform> m_Pose;
    std::vector<OrthogonalTransform> m_Skinning; // final skinning transform

    // Bone
    std::vector<Animation::BoneMotion> m_BoneMotions;
    std::vector<AffineTransform> m_BoneAttribute;

    std::map<std::wstring, uint32_t> m_MorphIndex;
    std::vector<Vector3> m_MorphDelta; // tempolar space to store morphed position delta
    enum { kMorphBase = 0 }; // m_MorphMotions's first slot is reserved as base (original) position data
    std::vector<Animation::MorphMotion> m_MorphMotions;
    std::vector<RigidBodyPtr> m_RigidBodies;
    std::vector<JointPtr> m_Joints;

    std::vector<XMFLOAT3> m_VertexMorphedPos; // temporal vertex positions which affected by face animation

    VertexBuffer m_AttributeBuffer;
    VertexBuffer m_PositionBuffer;
};

PmxInstant::Context::Context( PmxModel& model, PmxInstant* parent ) :
    m_Model( model ), m_bRightHand( true ), m_ModelTransform( kIdentity ), m_Parent( parent )
{
}

PmxInstant::Context::~Context()
{
    Clear();
}

void PmxInstant::Context::Clear()
{
    ASSERT( g_DynamicsWorld != nullptr );
    LeaveWorld( g_DynamicsWorld );

	m_AttributeBuffer.Destroy();
	m_PositionBuffer.Destroy();
}

void PmxInstant::Context::Draw( GraphicsContext& gfxContext, Visitor& visitor )
{
    std::vector<Matrix4> SkinData;
    SkinData.reserve( m_Skinning.size() );
    for (auto& orth : m_Skinning)
        SkinData.emplace_back( orth );
    auto numByte = GetVectorSize(SkinData);

    gfxContext.SetDynamicConstantBufferView( 1, numByte, SkinData.data(), { kBindVertex } );
    gfxContext.SetDynamicConstantBufferView( 2, sizeof(m_ModelTransform), &m_ModelTransform, { kBindVertex } );
	gfxContext.SetVertexBuffer( 0, m_AttributeBuffer.VertexBufferView() );
	gfxContext.SetVertexBuffer( 1, m_PositionBuffer.VertexBufferView() );
	gfxContext.SetIndexBuffer( m_Model.m_IndexBuffer.IndexBufferView() );

    for (auto& mesh : m_Model.m_Mesh)
	{
        if (!visitor.Visit( mesh ))
            continue;
        auto& material = m_Model.m_Materials[mesh.MaterialIndex];
        if (!visitor.Visit( material ))
            continue;
        material.SetTexture( gfxContext );
        gfxContext.SetDynamicConstantBufferView( 4, sizeof( material.CB ), &material.CB, { kBindVertex, kBindPixel } );
        gfxContext.DrawIndexed( mesh.IndexCount, mesh.IndexOffset, 0 );
	}
}

void PmxInstant::Context::DrawBone()
{
	auto numBones = m_BoneAttribute.size();
	for (auto i = 0; i < numBones; i++)
        PrimitiveUtility::Append( PrimitiveUtility::kBoneMesh, m_ModelTransform * m_Skinning[i] * m_BoneAttribute[i] );
}

bool PmxInstant::Context::LoadModel()
{
	m_VertexMorphedPos = m_Model.m_VertexPosition;
	m_AttributeBuffer.Create( m_Model.m_Name + L"_AttrBuf",
		static_cast<uint32_t>(m_Model.m_VertexAttribute.size()),
		sizeof( VertexProperty ),
		m_Model.m_VertexAttribute.data() );

	m_PositionBuffer.Create( m_Model.m_Name + L"_PosBuf",
		static_cast<uint32_t>(m_Model.m_VertexPosition.size()),
		sizeof( XMFLOAT3 ),
		m_Model.m_VertexPosition.data() );

	SetupSkeleton( m_Model.m_Bones );

    for (auto& it : m_Model.m_RigidBodies)
    {
        auto body = std::make_shared<RigidBody>();
        body->SetName( it.Name );
        body->SetNameEnglish( it.NameEnglish );
        body->SetBoneRef( BoneRef(m_Parent, it.BoneIndex) );
        body->SetCollisionGroupID( it.CollisionGroupID );
        body->SetCollisionMask( it.CollisionGroupMask );
        body->SetShapeType( static_cast<ShapeType>(it.Shape) );
        body->SetSize( it.Size );
        body->SetPosition( it.Position );
        const Quaternion rot( it.Rotation.x, it.Rotation.y, it.Rotation.z );
        body->SetRotation( rot );
        body->SetMass( it.Mass );
        body->SetLinearDamping( it.LinearDamping );
        body->SetAngularDamping( it.AngularDamping );
        body->SetRestitution( it.Restitution );
        body->SetFriction( it.Friction );
        body->SetObjectType( static_cast<ObjectType>(it.RigidType) );
        m_RigidBodies.push_back( std::move(body) );
    }

    for (auto i = 0; i < m_RigidBodies.size(); i++)
    {
        m_RigidBodies[i]->SetIndex( i );
        m_RigidBodies[i]->Build();
    }

    for (auto& it : m_Model.m_Joints)
    {
        auto body = std::make_shared<Joint>();
        body->SetName( it.Name );
        body->SetNameEnglish( it.NameEnglish );
        body->SetType( static_cast<JointType>(it.Type) );
        body->SetRigidBodyA( m_RigidBodies[it.RigidBodyIndexA] );
        body->SetRigidBodyB( m_RigidBodies[it.RigidBodyIndexB] );
        body->SetPosition( it.Position );
        const Quaternion rot( it.Rotation.x, it.Rotation.y, it.Rotation.z );
        body->SetRotation( rot );
        body->SetLinearLowerLimit( it.LinearLowerLimit );
        body->SetLinearUpperLimit( it.LinearUpperLimit );
        body->SetAngularLowerLimit( it.AngularLowerLimit );
        body->SetAngularUpperLimit( it.AngularUpperLimit );
        body->SetLinearStiffness( it.LinearStiffness );
        body->SetAngularStiffness( it.AngularStiffness );
        m_Joints.push_back( std::move( body ) );
    }

    for (auto i = 0; i < m_Joints.size(); i++)
    {
        m_Joints[i]->SetIndex( i );
        m_Joints[i]->Build();
    }

    ASSERT( g_DynamicsWorld != nullptr );
    JoinWorld( g_DynamicsWorld );

    return true;
}

bool PmxInstant::Context::LoadMotion( const std::wstring& motionPath )
{
	using namespace std;
	using namespace Animation;

	Utility::ByteArray ba = Utility::ReadFileSync( motionPath );
	Utility::ByteStream bs(ba);

	Vmd::VMD vmd;
	vmd.Fill( bs, m_bRightHand );
	if (!vmd.IsValid())
        return false;

    LoadBoneMotion( vmd.BoneFrames );

	for (auto& frame : vmd.FaceFrames)
	{
		MorphKeyFrame key;
		key.Frame = frame.Frame;
		key.Weight = frame.Weight;
        WARN_ONCE_IF(m_MorphIndex.count(frame.FaceName) <= 0, L"Can't find target morph on model: ");
        if (m_MorphIndex.count(frame.FaceName) > 0)
        {
            auto& motion = m_MorphMotions[m_MorphIndex[frame.FaceName]];
            motion.m_Name = frame.FaceName;
            motion.InsertKeyFrame( key );
        }
	}
	for (auto& face : m_MorphMotions )
		face.SortKeyFrame();
    return true;
}

void PmxInstant::Context::JoinWorld( btDynamicsWorld* world )
{
    if (world)
    {
        for (auto& it : m_RigidBodies)
            it->JoinWorld( world );

        for (auto& it : m_Joints)
            it->JoinWorld( world );
    }
}

void PmxInstant::Context::LeaveWorld( btDynamicsWorld* world )
{
    if (world)
    {
        for (auto& it : m_Joints)
            it->LeaveWorld( world );

        for (auto& it : m_RigidBodies)
            it->LeaveWorld( world );
    }
}

void PmxInstant::Context::LoadBoneMotion( const std::vector<Vmd::BoneFrame>& frames )
{
    if (frames.size() <= 0)
        return;
    auto& bones = m_Model.m_Bones;
    m_BoneMotions.resize( bones.size() );
	for (auto& frame : frames)
	{
        auto it = m_Model.m_BoneIndex.find(frame.BoneName);
        if (it == m_Model.m_BoneIndex.end())
			continue;
		Vector3 BoneTranslate(bones[it->second].Translate);

		Animation::BoneKeyFrame key;
		key.Frame = frame.Frame;
        // make offset motion to local translation 
		key.Local.SetTranslation( Vector3(frame.Offset) + BoneTranslate );
		key.Local.SetRotation( Quaternion( frame.Rotation ) );

		//
		// http://harigane.at.webry.info/201103/article_1.html
		//
		// X_x1, Y_x1, Z_x1, R_x1,
		// X_y1, Y_y1, Z_y1, R_y1,
		// X_x2, Y_x2, Z_x2, R_x2,
		// X_y2, Y_y2, Z_y2, R_y2,
		//
		// ... (duplicated values)
		//
		auto interp = reinterpret_cast<const char*>(&frame.Interpolation[0]);
		float scale = 1.0f / 127.0f;

		for (auto i = 0; i < 4; i++)
			key.BezierCoeff[i] = Vector4( interp[i], interp[i+4], interp[i+8], interp[i+12] ) * scale;

		m_BoneMotions[it->second].InsertKeyFrame( key );
	}

	for (auto& bone : m_BoneMotions )
		bone.SortKeyFrame();
}

// Use code from 'MMDAI'
// Copyright (c) 2010-2014  hkrn
void PmxInstant::Context::PerformTransform( uint32_t i )
{
    Quaternion orientation( kIdentity );
    if (m_Model.m_Bones[i].bInherentRotation) {
        uint32_t InherentRefIndex = m_Model.m_Bones[i].ParentInherentBoneIndex;
        ASSERT( InherentRefIndex >= 0 );
        PmxModel::Bone* parentBoneRef = &m_Model.m_Bones[InherentRefIndex];
        // If parent also Inherenet, then it has updated value. So, use cached one
        if (parentBoneRef->bInherentRotation) {
            orientation *= localInherentOrientations[InherentRefIndex];
        }
        else {
            orientation *= m_LocalPose[InherentRefIndex].GetRotation();
        }
        if (!Near( m_Model.m_Bones[i].ParentInherentBoneCoefficent, 1.f, FLT_EPSILON )) {
            orientation = Slerp( Quaternion( kIdentity ), orientation, m_Model.m_Bones[i].ParentInherentBoneCoefficent );
        }
        localInherentOrientations[i] = Normalize(orientation * m_LocalPose[i].GetRotation());
    }
    orientation *= m_LocalPose[i].GetRotation();
    orientation = Normalize( orientation );
    Vector3 translation( kZero );
    if (m_Model.m_Bones[i].bInherentTranslation) {
        uint32_t InherentRefIndex = m_Model.m_Bones[i].ParentInherentBoneIndex;
        ASSERT( InherentRefIndex >= 0 );
        PmxModel::Bone* parentBoneRef = &m_Model.m_Bones[InherentRefIndex];
        if (parentBoneRef) {
            if (parentBoneRef->bInherentTranslation) {
                translation += localInherentTranslations[InherentRefIndex];
            }
            else {
                translation += m_LocalPose[InherentRefIndex].GetTranslation();
            }
        }
        if (!Near( m_Model.m_Bones[i].ParentInherentBoneCoefficent, 1.f, FLT_EPSILON )) {
            translation *= Scalar(m_Model.m_Bones[i].ParentInherentBoneCoefficent);
        }
        localInherentTranslations[i] = translation;
    }
    translation += m_LocalPose[i].GetTranslation();
    m_LocalPose[i].SetRotation( orientation );
    m_LocalPose[i].SetTranslation( translation );
}

void PmxInstant::Context::Update( float kFrameTime )
{
    if (m_MorphMotions.size() > 0)
	{
		//
		// http://blog.goo.ne.jp/torisu_tetosuki/e/8553151c445d261e122a3a31b0f91110
		//
        auto elemByte = sizeof( decltype(m_MorphDelta)::value_type );
        memset( m_MorphDelta.data(), 0, elemByte*m_MorphDelta.size() );

		bool bUpdate = false;
		for (auto i = kMorphBase+1; i < m_MorphMotions.size(); i++)
		{
			auto& motion = m_MorphMotions[i];
			motion.Interpolate( kFrameTime );
			if (std::fabsf( motion.m_WeightPre - motion.m_Weight ) < 0.1e-2)
				continue;
			bUpdate = true;
			auto weight = motion.m_Weight;
			for (auto k = 0; k < motion.m_MorphVertices.size(); k++)
			{
                auto idx = motion.m_MorphIndices[k];
				m_MorphDelta[idx] += weight * motion.m_MorphVertices[k];
			}
		}
		if (bUpdate)
		{
            auto& baseFace = m_MorphMotions[kMorphBase];
			for (auto i = 0; i < m_MorphDelta.size(); i++)
                m_MorphDelta[i] += baseFace.m_MorphVertices[i];

			for (auto i = 0; i < m_MorphDelta.size(); i++)
				XMStoreFloat3( &m_VertexMorphedPos[baseFace.m_MorphIndices[i]], m_MorphDelta[i]);

			m_PositionBuffer.Create( m_Model.m_Name + L"_PosBuf",
				static_cast<uint32_t>(m_VertexMorphedPos.size()),
				sizeof( XMFLOAT3 ),
				m_VertexMorphedPos.data() );
		}
	}

	{
        //
        // in initialize m_LocalPoseDefault and in every motion data
        // position is already translated by offset from parent
        // so, local_pos = pos + offset
        //
        m_LocalPose = m_LocalPoseDefault;

        const size_t numMotions = m_BoneMotions.size();
		for (auto i = 0; i < numMotions; i++)
			m_BoneMotions[i].Interpolate( kFrameTime, m_LocalPose[i] );
        UpdatePose();
		for (auto& ik : m_Model.m_IKs)
            UpdateIK( ik );
		const size_t numBones = m_Model.m_Bones.size();
        for (auto i = 0; i < numBones; i++)
            PerformTransform( i );
        UpdatePose();
	}
}

void PmxInstant::Context::UpdateAfterPhysics( float kFrameTime )
{
    for (auto& it : m_RigidBodies)
        it->SyncLocalTransform();

    const size_t numBones = m_Model.m_Bones.size();
    for (auto i = 0; i < numBones; i++)
        m_Skinning[i] = m_Pose[i] * m_toRoot[i];
}

const OrthogonalTransform PmxInstant::Context::GetTransform( uint32_t i ) const
{
    return m_Pose[i];
}

void PmxInstant::Context::UpdateLocalTransform( uint32_t i )
{
    auto parentIndex = m_Model.m_Bones[i].Parent;
    if (parentIndex >= 0)
        m_Pose[i] = m_Pose[parentIndex] * m_LocalPose[i];
}

void PmxInstant::Context::SetLocalTransform( uint32_t i, const OrthogonalTransform& transform )
{
    m_Pose[i] = transform;
}

void PmxInstant::Context::UpdateChildPose( int32_t idx )
{
    auto parentIndex = m_Model.m_Bones[idx].Parent;
    if (parentIndex >= 0)
        m_Pose[idx] = m_Pose[parentIndex] * m_LocalPose[idx];
    else
        m_Pose[idx] = m_LocalPose[idx];

	for (auto c : m_Model.m_Bones[idx].Child)
		UpdateChildPose( c );
}

#pragma warning(push)
#pragma warning(disable: 4201)
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#pragma warning(pop)

inline Vector3 Convert(const glm::vec3& v )
{
    return Vector3( v.x, v.y, v.z );
}

inline glm::vec3 Convert(const Vector3& v )
{
    return glm::vec3( v.GetX(), v.GetY(), v.GetZ() );
}

inline glm::quat Convert( const Quaternion& q )
{
    XMFLOAT4 v;
    XMStoreFloat4( &v, q);
    glm::quat g;
    g.x = v.x;
    g.y = v.y;
    g.z = v.z;
    g.w = v.w;
    return g;
}

//
// Solve Constrainted IK
// Cyclic-Coordinate-Descent（CCD）
//
// http://d.hatena.ne.jp/edvakf/20111102/1320268602
//
void PmxInstant::Context::UpdateIK(const PmxModel::IKAttr& ik)
{
	auto GetPosition = [&]( int32_t index ) -> Vector3
	{
		return Vector3(m_Pose[index].GetTranslation());
	};

	// "effector" (Fixed)
	const auto ikBonePos = GetPosition( ik.BoneIndex );

	for (int n = 0; n < ik.NumIteration; n++)
	{
		// "effected" bone listed in order
		for (auto k = 0; k < ik.Link.size(); k++)
		{
            // TargetVector (link-target) is updated in each iteration
            // toward IkVector (link-ik)
            const auto ikTargetBonePos = GetPosition( ik.TargetBoneIndex );

			if (Length(ikBonePos - ikTargetBonePos) < FLT_EPSILON)
				return;

			auto linkIndex = ik.Link[k].BoneIndex;
			auto invLinkMtx = Invert( m_Pose[linkIndex] );

			// transform to child bone's local coordinate.
			auto ikTargetVec = Vector3( invLinkMtx * ikTargetBonePos );
			auto ikBoneVec = Vector3( invLinkMtx * ikBonePos );

        #if 1
			auto axis = Cross( ikBoneVec, ikTargetVec );
			auto axisLen = Length( axis );
			auto sinTheta = axisLen / Length( ikTargetVec ) / Length( ikBoneVec );
			if (sinTheta < 1.0e-5f)
				continue;

			// move angles in one iteration
			auto maxAngle = (k + 1) * ik.LimitedRadian * 4;
			auto theta = ASin( sinTheta );
			if (Dot( ikTargetVec, ikBoneVec ) < 0.f)
				theta = XM_PI - theta;
			if (theta > maxAngle)
				theta = maxAngle;

			auto rotBase = m_LocalPose[linkIndex].GetRotation();
			auto translate = m_LocalPose[linkIndex].GetTranslation();

			// To apply base coordinate system which it is base on, inverted theta direction
			Quaternion rotNext( axis, -theta );
			auto rotFinish = rotBase * rotNext;

			// Constraint IK, restrict rotation angle
			if (ik.Link[k].bLimit)
			{
            #define EXPERIMENT_IK
            #ifndef EXPERIMENT_IK
                // c = cos(theta / 2)
                auto c = XMVectorGetW( rotFinish );
                // s = sin(theta / 2)
                auto s = Sqrt( 1.0f - c*c );
                rotFinish = Quaternion( Vector4( s, 0, 0, c ) );
                if (!m_bRightHand)
                {
                    auto a = -std::asin( s );
                    rotFinish = Quaternion( Vector4( std::sin( a ), 0, 0, std::cos( a ) ) );
                }
            #else
                //
                // MMD-Agent PMDIK
                //
                /* when this is the first iteration, we force rotating to the maximum angle toward limited direction */
                /* this will help convergence the whole IK step earlier for most of models, especially for legs */
                if (n == 0)
                {
                    if (theta < 0.0f)
                        theta = -theta;
                    rotFinish = rotBase * Quaternion( Vector3( 1.0f, 0.f, 0.f ), theta );
				}
				else
				{
					//
					// Needed to stable IK result (esp. Ankle)
					// The value obtained from the test
					//
					const Scalar PMDMinRotX = 0.10f;
					auto next = rotNext.toEuler();
					auto base = rotBase.toEuler();

					auto sum = Clamp( next.GetX() + base.GetX(), PMDMinRotX, Scalar(XM_PI) );
					next = Vector3( sum - base.GetX(), 0.f, 0.f );
					rotFinish = rotBase * Quaternion( next.GetX(), next.GetY(), next.GetZ() );
				}
            #endif
			}
			m_LocalPose[linkIndex] = OrthogonalTransform( rotFinish, translate );
			UpdateChildPose( linkIndex );
        #else
            // IK link's coordinate, rotate target vector V_T to V_Ik
        #if 0
			// Use code from 'ray'
            // Copyright (c) 2015-2017  ray
			Vector3 srcLocal = Normalize(ikTargetVec);
			Vector3 dstLocal = Normalize(ikBoneVec);
			float rotationDotProduct = Dot(dstLocal, srcLocal);
            if (std::abs( rotationDotProduct - 1.f ) < FLT_EPSILON*100)
                return;
            float rotationAngle = ACos( rotationDotProduct );
            rotationAngle = min( ik.LimitedRadian*(k + 1), rotationAngle );

			if (rotationAngle < 0.0001f)
				continue;

			Vector3 rotationAxis = Cross(srcLocal, dstLocal);
			rotationAxis = Normalize(rotationAxis);

			Quaternion q0(rotationAxis, rotationAngle);
        #else
			auto axis = Cross( ikBoneVec, ikTargetVec );
			auto axisLen = Length( axis );
			auto sinTheta = axisLen / Length( ikTargetVec ) / Length( ikBoneVec );
			if (sinTheta < 1.0e-3f)
				continue;

			// angle to move in one iteration
			auto maxAngle = (k + 1) * ik.LimitedRadian * 4;
			float rotationAngle = ASin( sinTheta );
			if (Dot( ikTargetVec, ikBoneVec ) < 0.f)
				rotationAngle = XM_PI - rotationAngle;
            rotationAngle = min( ik.LimitedRadian*(k + 1), rotationAngle );

			// To apply base coordinate system which it is base on, inverted theta direction
			Quaternion q0( axis, -rotationAngle );
        #endif

			// if (false)
			if (ik.Link[k].bLimit)
			{
                Vector3 euler = Convert(glm::eulerAngles( Convert( q0 ) ));
                // due to rightHand min, max is swap needed
                Vector3 MinLimit = ik.Link[k].MaxLimit;
                MinLimit -= Vector3( Scalar( 0.005 ) );
                euler = Clamp( euler, MinLimit, ik.Link[k].MinLimit );
                q0 = Quaternion( euler.GetX(), euler.GetY(), euler.GetZ() );
			}

            auto& linkLocalPose = m_LocalPose[linkIndex];
			Quaternion qq = q0 * linkLocalPose.GetRotation();
			linkLocalPose = OrthogonalTransform( qq, linkLocalPose.GetTranslation() );
			UpdateChildPose( linkIndex );
        #endif
        }
	}
}

void PmxInstant::Context::UpdatePose()
{
    const size_t numBones = m_Model.m_Bones.size();
    for (auto i = 0; i < numBones; i++)
    {
        auto parentIndex = m_Model.m_Bones[i].Parent;
        if (parentIndex < numBones)
            m_Pose[i] = m_Pose[parentIndex] * m_LocalPose[i];
        else
            m_Pose[i] = m_LocalPose[i];
    }
}

void PmxInstant::Context::SetPosition( const Vector3& postion )
{
    m_ModelTransform = Matrix4::MakeTranslate( postion );
}

void PmxInstant::Context::SetupSkeleton( const std::vector<PmxModel::Bone>& Bones )
{
    auto bones = Bones;
    const int32_t numBones = static_cast<int32_t>(bones.size());
    m_Pose.resize( numBones );
    m_LocalPoseDefault.resize( numBones );
    m_toRoot.resize( numBones );
    m_Skinning.resize( numBones );
    for (auto i = 0; i < bones.size(); i++)
        m_LocalPoseDefault[i].SetTranslation( bones[i].Translate );
    m_LocalPose = m_LocalPoseDefault;
    for (auto i = 0; i < numBones; i++)
        m_Pose[i].SetTranslation( bones[i].Position );
    for (auto i = 0; i < numBones; i++)
        m_toRoot[i] = ~m_Pose[i];

    // set default skinning matrix
    m_Skinning.resize( numBones );

    localInherentOrientations.resize( numBones );
    localInherentTranslations.resize( numBones, Vector3(kZero) );

	std::vector<Vector3> GlobalPosition( numBones );
	m_BoneAttribute.resize( numBones );
	for ( auto i = 0; i < numBones; i++ )
	{
		auto DestinationIndex = m_Model.m_Bones[i].DestinationIndex;
		Vector3 DestinationOffset = m_Model.m_Bones[i].DestinationOffset;
		if (DestinationIndex >= 0)
			DestinationOffset = m_Model.m_Bones[DestinationIndex].Position - m_Model.m_Bones[i].Position;
		Vector3 diff = DestinationOffset;
		Scalar length = Length( diff );
		Quaternion Q = RotationBetweenVectors( Vector3( 0.0f, -1.0f, 0.0f ), diff );
		AffineTransform scale = AffineTransform::MakeScale( Vector3(0.05f, length, 0.05f) );
        // Move primitive bottom to origin
		m_BoneAttribute[i] = AffineTransform(Q, m_Model.m_Bones[i].Position) * scale;
	}
}

PmxInstant::PmxInstant( Model& model ) :
    m_Context( std::make_shared<Context>( dynamic_cast<PmxModel&>(model), this ) )
{
}

bool PmxInstant::LoadModel()
{
    return m_Context->LoadModel();
}

bool PmxInstant::LoadMotion( const std::wstring& motionPath )
{
    return m_Context->LoadMotion( motionPath );
}

void PmxInstant::Update( float deltaT )
{
    m_Context->Update( deltaT );
}

void PmxInstant::UpdateAfterPhysics( float deltaT )
{
    m_Context->UpdateAfterPhysics( deltaT );
}

const OrthogonalTransform PmxInstant::GetTransform( uint32_t i ) const
{
    return m_Context->GetTransform( i );
}

void PmxInstant::SetLocalTransform( uint32_t i, const OrthogonalTransform& transform )
{
    m_Context->SetLocalTransform( i, transform );
}

void PmxInstant::UpdateLocalTransform( uint32_t i )
{
    m_Context->UpdateLocalTransform( i );
}

void PmxInstant::Accept( Visitor& visitor )
{
    visitor.Visit( *this );
}

void PmxInstant::Render( GraphicsContext& Context, Visitor& visitor )
{
    m_Context->Draw( Context, visitor );
}

void PmxInstant::RenderBone( GraphicsContext& Context, Visitor& visitor )
{
    (Context), (visitor);
    m_Context->DrawBone();
}

BoneRef::BoneRef( PmxInstant* inst, uint32_t i ) : m_Instance( inst ), m_Index( i )
{
}

const OrthogonalTransform BoneRef::GetTransform() const
{
    ASSERT( m_Instance != nullptr );
    return m_Instance->GetTransform( m_Index );
}

void BoneRef::SetTransform( const btTransform& trnasform )
{
    AffineTransform local = Convert( trnasform );
    const OrthogonalTransform localOrth( Quaternion( local.GetBasis() ), local.GetTranslation() );
    SetTransform( localOrth );
}

void BoneRef::SetTransform( const OrthogonalTransform& transform )
{
    m_Instance->SetLocalTransform( m_Index, transform );
}

void BoneRef::UpdateLocalTransform()
{
    m_Instance->UpdateLocalTransform( m_Index );
}