﻿#include "stdafx.h"
#include "PmxModel.h"
#include "PmxInstant.h"
#include "Vmd.h"
#include "KeyFrameAnimation.h"
#include "PrimitiveUtility.h"
#include "Visitor.h"

using namespace Utility;
using namespace Math;
using namespace Graphics;

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

BoolVar s_bDrawBone( "Application/Model/Draw Bone", false );
BoolVar s_bDrawBoundingSphere( "Application/Model/Draw Bounding Shphere", false );
// If model is mixed with sky box, model's boundary is exculde by 's_ExcludeRange'
BoolVar s_bExcludeSkyBox( "Application/Model/Exclude Sky Box", true );
NumVar s_ExcludeRange( "Application/Model/Exclude Range", 1000.f, 500.f, 10000.f );

struct PmxInstant::Context final
{
    Context( PmxModel& model );
    ~Context();
    void Clear( void );
    void Draw( GraphicsContext& gfxContext, Visitor& visitor );
    void DrawBone();

    bool LoadModel();
    bool LoadMotion( const std::wstring& FilePath );

    void SetPosition( const Vector3& postion );
    void SetupSkeleton( const std::vector<PmxModel::Bone>& Bones );

    void Update( float kFrameTime );

protected:

    void LoadBoneMotion( const std::vector<Vmd::BoneFrame>& frames );
    void UpdateChildPose( int32_t idx );
    void UpdateIK( const PmxModel::IKAttr& ik );

    PmxModel& m_Model;
    bool m_bRightHand;
    Matrix4 m_ModelTransform;

    // Skinning
    std::vector<OrthogonalTransform> m_toRoot; // inverse inital pose ( inverse Rest)
    std::vector<OrthogonalTransform> m_LocalPose; // offset matrix
    std::vector<OrthogonalTransform> m_Pose;
    std::vector<OrthogonalTransform> m_Skinning; // final skinning transform

    // Bone
    std::vector<Animation::BoneMotion> m_BoneMotions;
    std::vector<AffineTransform> m_BoneAttribute;

    std::map<std::wstring, uint32_t> m_MorphIndex;
    std::vector<Vector3> m_MorphDelta; // tempolar space to store morphed position delta
    enum { kMorphBase = 0 }; // m_MorphMotions's first slot is reserved as base (original) position data
    std::vector<Animation::MorphMotion> m_MorphMotions;

    std::vector<XMFLOAT3> m_VertexMorphedPos; // temporal vertex positions which affected by face animation

    VertexBuffer m_AttributeBuffer;
    VertexBuffer m_PositionBuffer;
};

PmxInstant::Context::Context(PmxModel& model) :
    m_Model(model), m_bRightHand(true), m_ModelTransform(kIdentity)
{
}

PmxInstant::Context::~Context()
{
    Clear();
}

void PmxInstant::Context::Clear()
{
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

void PmxInstant::Context::LoadBoneMotion( const std::vector<Vmd::BoneFrame>& frames )
{
    if (frames.size() <= 0)
        return;

    auto bones = m_Model.m_Bones;
    int32_t numBones = static_cast<int32_t>(bones.size());
    m_BoneMotions.resize( numBones );
    m_LocalPose.resize( numBones );
    m_Pose.resize( numBones );
    m_toRoot.resize( numBones );
    for (auto i = 0; i < bones.size(); i++)
        m_LocalPose[i].SetTranslation( bones[i].Translate );

    std::vector<OrthogonalTransform> RestPose( numBones );
    for (auto i = 0; i < numBones; i++)
    {
        auto& bone = bones[i];
        auto parentIndex = bone.Parent;
        RestPose[i].SetTranslation( bone.Translate );
        if (parentIndex >= 0)
            RestPose[i] = RestPose[parentIndex] * RestPose[i];
    }

    for (auto i = 0; i < numBones; i++)
        m_toRoot[i] = ~RestPose[i];

	for (auto& frame : frames)
	{
        auto it = m_Model.m_BoneIndex.find(frame.BoneName);
        if (it == m_Model.m_BoneIndex.end())
			continue;
		Vector3 BoneTranslate(bones[it->second].Translate);

		Animation::BoneKeyFrame key;
		key.Frame = frame.Frame;
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

void PmxInstant::Context::SetPosition( const Vector3& postion )
{
    m_ModelTransform = Matrix4::MakeTranslate( postion );
}

void PmxInstant::Context::SetupSkeleton( const std::vector<PmxModel::Bone>& Bones )
{
    size_t numBones = Bones.size();

    // set default skinning matrix
    m_Skinning.resize( numBones );

	std::vector<Vector3> GlobalPosition( numBones );
	m_BoneAttribute.resize( numBones );
	for ( auto i = 0; i < numBones; i++ )
	{
		auto parentIndex = m_Model.m_Bones[i].Parent;
		Vector3 ParentPos = Vector3( kZero );
		if (parentIndex < numBones)
			ParentPos = GlobalPosition[parentIndex];

		Vector3 diff = m_Model.m_Bones[i].Translate;;
		Scalar length = Length( diff );
		Quaternion Q = RotationBetweenVectors( Vector3( 0.0f, -1.0f, 0.0f ), diff );
		AffineTransform scale = AffineTransform::MakeScale( Vector3(0.05f, length, 0.05f) );
        // Move primitive bottom to origin
		// AffineTransform alignToOrigin = AffineTransform::MakeTranslation( Vector3(0.0f, 0.5f * length, 0.0f) );
		GlobalPosition[i] = ParentPos + diff;
		m_BoneAttribute[i] = AffineTransform(Q, m_Model.m_Bones[i].Translate) * scale;
	}

}

void PmxInstant::Context::Update( float kFrameTime )
{
	if (m_BoneMotions.size() > 0)
	{
		size_t numBones = m_BoneMotions.size();
		for (auto i = 0; i < numBones; i++)
			m_BoneMotions[i].Interpolate( kFrameTime, m_LocalPose[i] );

		for (auto i = 0; i < numBones; i++)
		{
			auto parentIndex = m_Model.m_Bones[i].Parent;
			if (parentIndex >= 0)
				m_Pose[i] = m_Pose[parentIndex] * m_LocalPose[i];
			else
				m_Pose[i] = m_LocalPose[i];
		}

    #if 0
		for (auto& ik : m_Model.m_IKs)
			UpdateIK( ik );
    #endif

		for (auto i = 0; i < numBones; i++)
			m_Skinning[i] = m_Pose[i] * m_toRoot[i];
	}

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
}

//
// Solve Constrainted IK
// Cyclic-Coordinate-Descent（CCD）
//
// http://d.hatena.ne.jp/edvakf/20111102/1320268602
// Game programming gems 3 Constrained Inverse Kinematics - Jason Weber
//
void PmxInstant::Context::UpdateIK(const PmxModel::IKAttr& ik)
{
	auto GetPosition = [&]( int32_t index ) -> Vector3
	{
		return Vector3(m_Pose[index].GetTranslation());
	};

	// "effector" (Fixed)
	const auto ikBonePos = GetPosition( ik.BoneIndex );
    const auto ikTargetBonePos = GetPosition( ik.TargetBoneIndex );

	for (int n = 0; n < ik.NumIteration; n++)
	{
		// "effected" bone list in order
		for (auto k = 0; k < ik.Link.size(); k++)
		{
			auto childIndex = ik.Link[k].BoneIndex;
			auto invLinkMtx = Invert( m_Pose[childIndex] );

			//
			// transform to child bone's local coordinate.
			// note that even if pos is vector3 type, transformed by affine tranform.
			//
			auto ikTargetVec = Vector3( invLinkMtx * ikTargetBonePos );
			auto ikBoneVec = Vector3( invLinkMtx * ikBonePos );

            // rotated axis
			auto axis = Cross( ikBoneVec, ikTargetVec );
			auto axisLen = Length( axis );
			auto sinTheta = axisLen / Length( ikTargetVec ) / Length( ikBoneVec );
			if (sinTheta < 1.0e-3f)
				continue;

			// angles moved in one iteration
        #if 0
			auto maxAngle = (k + 1) * ik.LimitedRadian * 4;
        #else
			auto maxAngle = ik.LimitedRadian;
        #endif
			auto theta = ASin( sinTheta );
			if (Dot( ikTargetVec, ikBoneVec ) < 0.f)
				theta = XM_PI - theta;
			if (theta > maxAngle)
				theta = maxAngle;

			auto rotBase = m_LocalPose[childIndex].GetRotation();
			auto translate = m_LocalPose[childIndex].GetTranslation();

			// To apply base coordinate system which it is base on, inverted theta direction
			Quaternion rotNext( axis, -theta );
			auto rotFinish = rotBase * rotNext;

			// Constraint IK, restrict rotation angle
			if (false) // ik.Link[k].bLimit)
			{
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
			}
			m_LocalPose[childIndex] = OrthogonalTransform( rotFinish, translate );
			UpdateChildPose( childIndex );
		}
	}
}

PmxInstant::PmxInstant( Model& model ) :
    m_Context( std::make_shared<Context>( dynamic_cast<PmxModel&>(model) ) )
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
    SceneNode::Update( deltaT );
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
