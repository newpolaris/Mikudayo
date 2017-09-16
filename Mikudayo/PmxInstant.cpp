#include "stdafx.h"
#include "PmxModel.h"
#include "PmxInstant.h"
#include "Vmd.h"
#include "KeyFrameAnimation.h"
#include "FxContainer.h"
#include "SoftBodyManager.h"
#include "Bullet/Physics.h"
#include "Bullet/BaseSoftBody.h"

using namespace Utility;
using namespace Math;
using namespace Graphics;

namespace {

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

struct PmxInstant::Context final
{
    Context( const PmxModel& model );
    ~Context();
    void Clear( void );
    void Draw( GraphicsContext& gfxContext, const std::string& technique );

    bool LoadModel();
    bool LoadMotion( const std::wstring& FilePath );

    void SetPosition( const Vector3& postion );
    void SetupSkeleton();

    void Update( float kFrameTime );

protected:

    void LoadBoneMotion( const std::vector<Vmd::BoneFrame>& frames );

    const PmxModel& m_Model;
    bool m_bRightHand;
    Matrix4 m_ModelTransform;

    // Skinning
    std::vector<AffineTransform> m_toRoot; // inverse inital pose ( inverse Rest)
    std::vector<AffineTransform> m_LocalPose; // local offset
    std::vector<AffineTransform> m_Pose; // hieracical offset
    std::vector<AffineTransform> m_Skinning; // final skinning transform

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

    std::shared_ptr<Physics::BaseSoftBody> m_SoftBody;
};

PmxInstant::Context::Context(const PmxModel& model) :
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

    if (m_SoftBody)
        m_SoftBody->LeaveWorld( Physics::g_DynamicsWorld );
}

void PmxInstant::Context::Draw( GraphicsContext& gfxContext, const std::string& technique )
{
    std::vector<Matrix4> SkinData;
    SkinData.reserve( m_Skinning.size() );
    for (auto& orth : m_Skinning)
        SkinData.emplace_back( orth );
    auto numByte = GetVectorSize(SkinData);

    gfxContext.SetDynamicConstantBufferView( 1, numByte, SkinData.data(), { kBindVertex } );
    gfxContext.SetDynamicConstantBufferView( 2, sizeof(m_ModelTransform), &m_ModelTransform, { kBindVertex, kBindGeometry } );
	gfxContext.SetVertexBuffer( 0, m_AttributeBuffer.VertexBufferView() );
	gfxContext.SetVertexBuffer( 1, m_PositionBuffer.VertexBufferView() );
	gfxContext.SetIndexBuffer( m_Model.m_IndexBuffer.IndexBufferView() );
	for (auto& mesh : m_Model.m_Mesh)
	{
        auto& material = m_Model.m_Materials[mesh.MaterialIndex];
        if (!material.SetTexture( gfxContext ))
            continue;
        if (!material.Techniques)
            continue;
        material.Techniques->SetSampler( gfxContext );
        auto numPass = material.Techniques->FindTechnique(technique);
        for (auto i = 0U; i < numPass; i++)
        {
            material.Techniques->SetPass( gfxContext, technique, i );
            gfxContext.SetDynamicConstantBufferView( 3, sizeof(material.CB), &material.CB, { kBindPixel } );
            gfxContext.DrawIndexed( mesh.IndexCount, mesh.IndexOffset, 0 );
        }
	}
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

	SetupSkeleton();

    m_SoftBody = SoftBodyManager::GetInstance(m_Model.m_SoftBodyName);
    if (m_SoftBody)
        m_SoftBody->JoinWorld( Physics::g_DynamicsWorld );

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
        WARN_ONCE_IF(m_MorphIndex.count(frame.FaceName) <= 0,
            L"Can't find target morph " + frame.FaceName + L" on model: " + m_Model.m_Name );
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

void PmxInstant::Context::SetPosition( const Vector3& postion )
{
    m_ModelTransform = Matrix4::MakeTranslate( postion );
}

void PmxInstant::Context::SetupSkeleton()
{
    m_LocalPose = m_Model.m_LocalPose;
    m_Pose = m_Model.m_Pose;
    for (auto& pose : m_Pose)
        m_toRoot.push_back(OrthoInvert(pose));

    // set default skinning matrix
    for (size_t i = 0; i < m_Pose.size(); i++)
        m_Skinning.push_back(AffineTransform(kIdentity));
}

void PmxInstant::Context::Update( float kFrameTime )
{
    if (m_SoftBody)
    {
        m_SoftBody->GetSoftBodyPose( m_Pose );
		for (auto i = 0; i < m_Pose.size(); i++)
			m_Skinning[i] = m_Pose[i] * m_toRoot[i];
    }

	if (m_BoneMotions.size() > 0)
	{
		size_t numBones = m_BoneMotions.size();
		for (auto i = 0; i < numBones; i++)
        {
            OrthogonalTransform localPose;
			m_BoneMotions[i].Interpolate( kFrameTime, localPose );
            m_LocalPose[i] = localPose;
        }
        m_Pose = m_LocalPose;
		for (auto i = 0; i < numBones; i++)
		{
			const auto parentIndex = m_Model.m_Bones[i].Parent;
			if (parentIndex >= 0)
				m_Pose[i] = m_Pose[parentIndex] * m_Pose[i];
		}
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

PmxInstant::PmxInstant( const Model& model ) :
    m_Context( std::make_shared<Context>( dynamic_cast<const PmxModel&>(model) ) )
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

void PmxInstant::DrawColor( GraphicsContext& Context )
{
    m_Context->Draw( Context, "t0");
}
