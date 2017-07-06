#include "MikuModel.h"

#include <string>
#include <vector>
#include <map>
#include <DirectXMath.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "TextureManager.h"
#include "FileUtility.h"
#include "Utility.h"
#include "Mapping.h"
#include "Encoding.h"
#include "GeometryGenerator.h"

#include "CompiledShaders/BoneVS.h"
#include "CompiledShaders/BonePS.h"

using namespace Graphics;
using namespace DirectX;

BoolVar s_EnableDrawBone("Application/Draw Bone", false);

MikuModel::MikuModel( bool bRightHand ) : m_bRightHand( bRightHand ) 
{
}

MikuModel::~MikuModel()
{
    Clear();
}

void MikuModel::Load()
{
    if (!m_Model.empty())
        LoadPmd( m_Model, m_bRightHand );

	if (!m_Motion.empty())
        LoadVmd( m_Motion, m_bRightHand );

    LoadBone();
}

void MikuModel::SetModel( const std::wstring& model )
{
    m_Model = model;
}

void MikuModel::SetMotion( const std::wstring& motion )
{
    m_Motion = motion;
}

void MikuModel::LoadPmd( const std::wstring& modelPath, bool bRightHand )
{
	using Path = fs::path;
	Path model ( modelPath );

	bool bZipArchive = Utility::isZip( model );

	if (bZipArchive)
	{
		auto archive = std::make_shared<Utility::ZipArchive>( modelPath );
		auto Filenames = archive->GetFileList();

		Path pmdname;
		for (auto& name : Filenames)
		{
			auto ext = Path(name).extension().generic_wstring();
			if (boost::to_lower_copy(ext) == L".pmd")
			{
				pmdname = name;
				break;
			}
		}
		ASSERT( !pmdname.empty() );
		LoadPmd( archive, pmdname, bRightHand );

	}
	else
	{
		auto archive = std::make_shared<Utility::RelativeFile>( model.parent_path() );
		LoadPmd( archive, model.filename(), bRightHand );
	}
}

void MikuModel::LoadPmd( Utility::ArchivePtr archive, fs::path pmdPath, bool bRightHand )
{
	auto is = archive->GetFile( pmdPath );

	Pmd::PMD pmd;
	pmd.Fill( *is, bRightHand );
	ASSERT( pmd.IsValid() );

	// 
	// raw: decode with system default. 
	//      if system default is not shift-jis it will display with corrupted charactor.
	//      but, it will also happens when extract from zip archive. so, can find the file
	// unicode: decode with shift-jis
	//
	auto LoadTexture = []( Utility::ArchivePtr archive, std::string raw, std::wstring unicode, bool bSRGB )
		-> D3D11_SRV_HANDLE
	{
		//
		// zip format does not support unicode.
		// So, it is convenient to use unified ecoding, system default
        //
        std::vector<char> buf;
		// Try raw name
		auto key = archive->GetKeyName( raw );
		auto is = archive->GetFile( key );
        if (!is) 
        {
            // If not, try unicode interpreted name
            key = archive->GetKeyName( unicode );
            is = archive->GetFile( key );
        }
        if (!is) {
           std::ostringstream ss;
           ss << is->rdbuf();
           const std::string& s = ss.str();
           std::vector<char> buf(s.begin(), s.end());
        }
		auto texture = TextureManager::LoadFromStream( key.generic_wstring(), *is, bSRGB );

		if (!texture->IsValid())
        {
            // If not, try default provided texture in MMD (toon01.bmp)
            auto toon = fs::path( "toon" ) / raw;
            texture = TextureManager::LoadFromFile( toon.generic_wstring(), bSRGB );
        }
		if (!texture->IsValid())
			return nullptr;
		return texture->GetSRV();
	};

	struct Attribute
	{
		XMFLOAT3 Normal;
		XMFLOAT2 UV;
		uint16_t Bone_id[2];
		uint8_t  Bone_weight;
		uint8_t  Edge_flat;
	};

	std::vector<Attribute> attributes( pmd.m_Vertices.size() );
	m_VertexPos.resize( pmd.m_Vertices.size() );
	for (auto i = 0; i < pmd.m_Vertices.size(); i++)
	{
		m_VertexPos[i] = pmd.m_Vertices[i].Pos;
		attributes[i].Normal = pmd.m_Vertices[i].Normal;
		attributes[i].UV = pmd.m_Vertices[i].UV;
		attributes[i].Bone_id[0] = pmd.m_Vertices[i].Bone_id[0];
		attributes[i].Bone_id[1] = pmd.m_Vertices[i].Bone_id[1];
		attributes[i].Bone_weight = pmd.m_Vertices[i].Bone_weight;
		attributes[i].Edge_flat = pmd.m_Vertices[i].Edge_flat;
	}
	m_VertexMorphedPos = m_VertexPos;

	m_Name = pmd.m_Header.Name;

	m_AttributeBuffer.Create( m_Name + L"_AttrBuf", 
		static_cast<uint32_t>(attributes.size()),
		sizeof( Attribute ),
		attributes.data() );

	m_PositionBuffer.Create( m_Name + L"_PosBuf", 
		static_cast<uint32_t>(m_VertexPos.size()),
		sizeof( XMFLOAT3 ),
		m_VertexPos.data() );

	m_IndexBuffer.Create( pmd.m_Header.Name + L"_IndexBuf",
		static_cast<uint32_t>(pmd.m_Indices.size()),
		sizeof( pmd.m_Indices[0] ),
		pmd.m_Indices.data() );

	uint32_t IndexOffset = 0;
	for (auto& material : pmd.m_Materials)
	{
		Mesh mesh = {};

		MaterialCB mat = {};
		mat.Diffuse = material.Diffuse;
		mat.SpecularPower = material.SpecularPower;
		mat.Ambient = material.Ambient;

		mesh.IndexCount = material.FaceVertexCount;
		mesh.IndexOffset = IndexOffset;
		IndexOffset += material.FaceVertexCount;

		if (!material.TextureRaw.empty())
			mesh.Texture[kTextureDiffuse] = LoadTexture( archive, material.TextureRaw, material.Texture, true );
		if (!material.SphereRaw.empty()) {
			// 
			// https://learnmmd.com/http:/learnmmd.com/pmd-editor-basics-sph-and-spa-files-add-sparkle/
			//
			// spa adds to main texture and sph multiplies.
			// However, spa does not use sRGB because it is generally 
			// used as a brightness image.
			//
			bool sRGB = true;
			if (std::string::npos != material.SphereRaw.rfind(".spa"))
				sRGB = false;
			mesh.Texture[kTextureSphere] = LoadTexture( archive, material.SphereRaw, material.Texture, sRGB );
		}
		if (material.ToonIndex < pmd.m_ToonTextureRawList.size())
		{
			auto toonRaw = pmd.m_ToonTextureRawList[material.ToonIndex];
			auto toon = pmd.m_ToonTextureList[material.ToonIndex];
			mesh.Texture[kTextureToon] = LoadTexture( archive, toonRaw, toon, true );
		}
		if (mesh.Texture[kTextureToon])
			mat.bUseToon = TRUE;
		if (mesh.Texture[kTextureDiffuse])
			mat.bUseTexture = TRUE;
		if (mesh.Texture[kTextureSphere])
			mat.SphereOperation = material.SphereOperation;

		mesh.Material = mat;

		mesh.bEdgeFlag = material.EdgeFlag > 0;

		m_Mesh.push_back(mesh);
	}

	size_t numBones = pmd.m_Bones.size();
	SetBoneNum( numBones );
	for (auto i = 0; i < numBones; i++) 
	{
		auto& boneData = pmd.m_Bones[i];

		m_Bones[i].Name = boneData.Name;
		m_BoneParent[i] = boneData.ParentBoneIndex;
		if (boneData.ParentBoneIndex < numBones)
			m_BoneChild[boneData.ParentBoneIndex].push_back( i );

		Vector3 headPos = boneData.BoneHeadPosition;
		auto parentPos = Vector3( 0.0f, 0.0f, 0.0f );

		if( boneData.ParentBoneIndex < numBones )
			parentPos = pmd.m_Bones[boneData.ParentBoneIndex].BoneHeadPosition;

		m_Bones[i].Translate = headPos - parentPos;

		m_BoneIndex[boneData.Name] = i;
	}
    
    m_Skinning.resize( numBones );
    m_SkinningDual.resize( numBones );
    for ( auto i = 0; i < numBones; i++)
        m_SkinningDual[i] = OrthogonalTransform();

	m_IKs = pmd.m_IKs;

	m_MorphMotions.resize( pmd.m_Faces.size() );
	for ( auto i = 0; i < pmd.m_Faces.size(); i++ )
	{
		auto& morph = pmd.m_Faces[i];
		m_MorphIndex[morph.Name] = i;
        auto numVertices = morph.FaceVertices.size();

        auto& motion = m_MorphMotions[i];
        motion.m_MorphVertices.reserve( numVertices );
        motion.m_MorphVertices.reserve( numVertices );
		for (auto& vert : morph.FaceVertices)
        {
			motion.m_MorphIndices.push_back( vert.Index );
			motion.m_MorphVertices.push_back( vert.Position );
        }
	}
    if (m_MorphMotions.size() > 0)
        m_MorphDelta.resize( m_MorphMotions[kMorphBase].m_MorphIndices.size() );
}

void MikuModel::LoadVmd( const std::wstring& motionPath, bool bRightHand )
{
	using namespace std;
	using namespace Animation;
	
	Utility::ByteArray ba = Utility::ReadFileSync( motionPath );
	Utility::ByteStream bs(ba);

	Vmd::VMD vmd;
	vmd.Fill( bs, bRightHand );
	ASSERT( vmd.IsValid() );

    LoadBoneMotion( vmd.BoneFrames );

	for (auto& frame : vmd.FaceFrames)
	{
		MorphKeyFrame key;
		key.Frame = frame.Frame;
		key.Weight = frame.Weight;
		key.Weight = frame.Weight;

        WARN_ONCE_IF(m_MorphIndex.count(frame.FaceName) <= 0, L"Can't find target morph on model: " + m_Model);
        if (m_MorphIndex.count(frame.FaceName) > 0)
        {
            auto& motion = m_MorphMotions[m_MorphIndex[frame.FaceName]];
            motion.m_Name = frame.FaceName;
            motion.InsertKeyFrame( key );
        }
	}

	for (auto& face : m_MorphMotions )
		face.SortKeyFrame();

	for (auto& frame : vmd.CameraFrames)
	{
		CameraKeyFrame keyFrame;
		keyFrame.Frame = frame.Frame;
		keyFrame.Data.bPerspective = frame.TurnOffPerspective == 0;
		keyFrame.Data.Distance = frame.Distance;
		keyFrame.Data.FovY = frame.ViewAngle / XM_PI;
		keyFrame.Data.Rotation = Quaternion( frame.Rotation.y, frame.Rotation.x, frame.Rotation.z );
		keyFrame.Data.Position = frame.Position;

		//
		// http://harigane.at.webry.info/201103/article_1.html
		// 
		auto interp = reinterpret_cast<const char*>(&frame.Interpolation[0]);
		float scale = 1.0f / 127.0f;

		for (auto i = 0; i < 6; i++)
			keyFrame.BezierCoeff[i] = Vector4( interp[i], interp[i+2], interp[i+1], interp[i+3] ) * scale;

		m_CameraMotion.InsertKeyFrame( keyFrame );
	}
}

void MikuModel::SetBoneNum( size_t numBones )
{
	m_BoneParent.resize( numBones );
	m_BoneChild.resize( numBones );
	m_Bones.resize( numBones );
}

void MikuModel::LoadBoneMotion( const std::vector<Vmd::BoneFrame>& frames )
{
    if (frames.size() <= 0) 
        return;

    int32_t numBones = static_cast<int32_t>(m_Bones.size());

    m_BoneMotions.resize( numBones );
    m_Pose.resize( numBones );
    m_LocalPose.resize( numBones );
    m_toRoot.resize( numBones );
    m_Skinning.resize( numBones );
    m_SkinningDual.resize( numBones );

    for (auto i = 0; i < numBones; i++)
    {
        auto& bone = m_Bones[i];
        auto& meshBone = m_BoneMotions[i];

        meshBone.bLimitXAngle = false;

        //
        // In PMD Model minIK, maxIK is not manually given. 
        // But, bone name that contains 'knee'('ひざ') has constraint
        // that can move only in x axis and outer angle (just like human knee)
        // If this constraint is not given, knee goes forward just like 
        // the following vmd motion. http://www.nicovideo.jp/watch/sm18737664 
        //
        if (std::string::npos != bone.Name.find( L"ひざ" ))
            meshBone.bLimitXAngle = true;
    }

    for (auto i = 0; i < m_Bones.size(); i++)
        m_LocalPose[i].SetTranslation( m_Bones[i].Translate );

    std::vector<OrthogonalTransform> RestPose( numBones );
    for (auto i = 0; i < numBones; i++)
    {
        auto& bone = m_Bones[i];
        auto& parent = m_BoneParent[i];

        RestPose[i].SetTranslation( bone.Translate );
        if (parent < numBones)
            RestPose[i] = RestPose[parent] * RestPose[i];
    }

    for (auto i = 0; i < numBones; i++)
        m_toRoot[i] = ~RestPose[i];

	for (auto& frame : frames)
	{
		if ( m_BoneIndex.count( frame.BoneName ) == 0)
			continue;

		Vector3 BoneTranslate(m_Bones[m_BoneIndex[frame.BoneName]].Translate);

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

		m_BoneMotions[m_BoneIndex[frame.BoneName]].InsertKeyFrame( key );
	}

	for (auto& bone : m_BoneMotions )
		bone.SortKeyFrame();
}

void MikuModel::LoadBone()
{
	std::vector<GeometryGenerator::Vertex> Vertices;
	std::vector<uint32_t> Indices;
	
    GeometryGenerator geoGen;
	{
		GeometryGenerator::MeshData sphere = geoGen.CreateCylinder( 2.0f, 0.1f, 1.0f, 5, 1 );
		int32_t baseIndex = static_cast<int32_t>(Indices.size());
		int32_t baseVertex = static_cast<int32_t>(Vertices.size());
		std::copy( sphere.Vertices.begin(), sphere.Vertices.end(), std::back_inserter( Vertices ) );
		std::copy( sphere.Indices32.begin(), sphere.Indices32.end(), std::back_inserter( Indices ) );
		SubmeshGeometry mesh;
		mesh.IndexCount = static_cast<int32_t>(sphere.Indices32.size());
		mesh.IndexOffset = baseIndex;
		mesh.VertexOffset = baseVertex;
		m_BoneMesh = mesh;
	}

	m_BoneVertexBuffer.Create( L"BoneVertex", 
		static_cast<uint32_t>(Vertices.size()),
		sizeof( Vertices[0] ),
		Vertices.data() );

	m_BoneIndexBuffer.Create( L"BoneIndex",
		static_cast<uint32_t>(Indices.size()),
		sizeof( Indices[0] ),
		Indices.data() );

	std::vector<InputDesc> boneInputDesc 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	D3D11_RASTERIZER_DESC RasterizerWire = RasterizerDefault;
	RasterizerWire.FillMode = D3D11_FILL_WIREFRAME;

	m_BonePSO.SetInputLayout( static_cast<UINT>(boneInputDesc.size()), boneInputDesc.data() );
	m_BonePSO.SetVertexShader( MY_SHADER_ARGS( g_pBoneVS ) );
	m_BonePSO.SetPixelShader( MY_SHADER_ARGS( g_pBonePS ) );
	m_BonePSO.SetDepthStencilState( DepthStateDisabled );
	m_BonePSO.SetRasterizerState( RasterizerWire );
	m_BonePSO.Finalize();

	auto numBone = m_Bones.size();

	std::vector<Vector3> GlobalPosition( numBone );
	m_BoneAttribute.resize( numBone );

	for ( auto i = 0; i < numBone; i++ )
	{
		auto parentIndex = m_BoneParent[i];
		Vector3 ParentPos = Vector3( kZero );
		if (parentIndex < numBone) 
			ParentPos = GlobalPosition[parentIndex];

		Vector3 diff = m_Bones[i].Translate;
		Scalar l = Length( diff );
		XMMATRIX S = XMMatrixScaling( 0.05f, l, 0.05f );
		Quaternion Q = RotationBetweenVectors( Vector3( 0.0f, 1.0f, 0.0f ), diff );
		XMMATRIX R = XMMatrixRotationQuaternion( Q );
		XMMATRIX Z = XMMatrixTranslation( 0.0f, 0.5f * l, 0.0f );
		XMMATRIX T = XMMatrixTranslationFromVector( ParentPos );

		GlobalPosition[i] = ParentPos + diff;
		m_BoneAttribute[i] = XMMatrixMultiply( XMMatrixMultiply( XMMatrixMultiply( S, Z ), R ), T );
	}
}

void MikuModel::Clear()
{
	m_AttributeBuffer.Destroy();
	m_PositionBuffer.Destroy();
	m_IndexBuffer.Destroy();
	m_BonePSO.Destroy();
	m_BoneIndexBuffer.Destroy();
	m_BoneVertexBuffer.Destroy();
}

void MikuModel::UpdateChildPose( int32_t idx )
{
	auto numBone = m_BoneMotions.size();
	auto parentIndex = m_BoneParent[idx];

	if (parentIndex < numBone)
		m_Pose[idx] = m_Pose[parentIndex] * m_LocalPose[idx];

	for (auto c : m_BoneChild[idx])
		UpdateChildPose( c );
}

void MikuModel::Update( float kFrameTime )
{
	if (m_BoneMotions.size() > 0)
	{
		size_t numBones = m_BoneMotions.size();
		for (auto i = 0; i < numBones; i++) 
			m_BoneMotions[i].Interpolate( kFrameTime, m_LocalPose[i] );
		
		for (auto i = 0; i < numBones; i++)
		{
			auto parentIndex = m_BoneParent[i];

			if (parentIndex < numBones)
				m_Pose[i] = m_Pose[parentIndex] * m_LocalPose[i];
			else
				m_Pose[i] = m_LocalPose[i];
		}

		for (auto& ik : m_IKs)
			UpdateIK( ik );

		for (auto i = 0; i < numBones; i++) 
			m_Skinning[i] = m_Pose[i] * m_toRoot[i];

		for (auto i = 0; i < numBones; i++) 
            m_SkinningDual[i] = m_Skinning[i];
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

			m_PositionBuffer.Create( m_Name + L"_PosBuf", 
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
void MikuModel::UpdateIK(const Pmd::IK& ik)
{
	auto GetPosition = [&]( int32_t index ) -> Vector3
	{
		return Vector3(m_Pose[index].GetTranslation());
	};

	// "effector" (Fixed)
	const auto ikBonePos = GetPosition( ik.IkBoneIndex );

	for (int n = 0; n < ik.IkNumIteration; n++)
	{
		// "effected" bone list in order
		for (auto k = 0; k < ik.IkLinkBondIndexList.size(); k++)
		{
			auto childIndex = ik.IkLinkBondIndexList[k];
			auto ikTargetBonePos = GetPosition( ik.IkTargetBonIndex );
			auto invLinkMtx = Invert( m_Pose[childIndex] );

			//
			// transform to child bone's local coordinate.
			// note that even if pos is vector3 type, it is calcurated by affine tranform.
			//
			auto ikTargetVec = Vector3( invLinkMtx * ikTargetBonePos );
			auto ikBoneVec = Vector3( invLinkMtx * ikBonePos );

			auto axis = Cross( ikBoneVec, ikTargetVec );
			auto axisLen = Length( axis );
			auto sinTheta = axisLen / Length( ikTargetVec ) / Length( ikBoneVec );
			if (sinTheta < 1.0e-3f)
				continue;

			// angle to move in one iteration
			auto maxAngle = (k + 1) * ik.IkLimitedRadian * 4;
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
			if (m_BoneMotions[childIndex].bLimitXAngle)
			{
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
			m_LocalPose[childIndex] = OrthogonalTransform( rotFinish, translate );
			UpdateChildPose( childIndex );
		}
	}
}

void MikuModel::Draw( GraphicsContext& gfxContext, eObjectFilter Filter )
{
    auto elemByte = sizeof( decltype(m_SkinningDual)::value_type );
    auto numByte = elemByte * m_SkinningDual.size();
    gfxContext.SetDynamicConstantBufferView( 1, numByte, m_SkinningDual.data(), { kBindVertex } );
    gfxContext.SetDynamicConstantBufferView( 2, sizeof(m_ModelTrnasform), &m_ModelTrnasform, { kBindVertex } );
	gfxContext.SetVertexBuffer( 0, m_AttributeBuffer.VertexBufferView() );
	gfxContext.SetVertexBuffer( 1, m_PositionBuffer.VertexBufferView() );
	gfxContext.SetIndexBuffer( m_IndexBuffer.IndexBufferView() );

	for (auto mesh: m_Mesh)
	{
		bool bOpaque = Filter & kOpaque && !mesh.isTransparent();
		bool bTransparent = Filter & kTransparent && mesh.isTransparent();
		if (!bOpaque && !bTransparent) continue;

		gfxContext.SetDynamicDescriptors( 0, _countof( mesh.Texture ), mesh.Texture, { kBindPixel } );
		gfxContext.SetDynamicConstantBufferView( 0, sizeof(mesh.Material), &mesh.Material, { kBindPixel } );
		gfxContext.SetDynamicSampler( 0, SamplerLinearWrap, kBindPixel );
		gfxContext.SetDynamicSampler( 1, SamplerLinearClamp, kBindPixel );
		gfxContext.DrawIndexed( mesh.IndexCount, mesh.IndexOffset, 0 );
	}
}

void MikuModel::DrawBone( GraphicsContext& gfxContext )
{
    if (!s_EnableDrawBone) return;
    
	gfxContext.SetPipelineState( m_BonePSO );
	gfxContext.SetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	gfxContext.SetVertexBuffer( 0, m_BoneVertexBuffer.VertexBufferView() );
	gfxContext.SetIndexBuffer( m_BoneIndexBuffer.IndexBufferView() );

	auto numBones = m_BoneAttribute.size();

	for (auto i = 0; i < numBones; i++)
	{
		XMMATRIX mat = XMMatrixMultiply( m_BoneAttribute[i], Matrix4(m_Skinning[i]) );
		gfxContext.SetDynamicConstantBufferView( 1, sizeof(mat), &mat, { kBindVertex } );
        gfxContext.SetDynamicConstantBufferView( 2, sizeof(m_ModelTrnasform), &m_ModelTrnasform, { kBindVertex } );
		gfxContext.DrawIndexed( m_BoneMesh.IndexCount, m_BoneMesh.IndexOffset, m_BoneMesh.VertexOffset );
	}
}