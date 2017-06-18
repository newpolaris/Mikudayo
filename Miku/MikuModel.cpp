#include "MikuModel.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <sstream>
#include <regex>
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

const auto MinIK = Vector3( -XM_PI, -XM_PI, -XM_PI );
const auto MaxIK = Vector3( XM_PI, XM_PI, XM_PI );
const auto EpsIK = Vector3( 0.02f, 0.02f, 0.02f ); // near 1.4 degree

namespace Pmd {
	std::vector<InputDesc> InputDescriptor
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_ID", 0, DXGI_FORMAT_R16G16_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHT", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "EDGE_FLAT", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
}

MikuModel::MikuModel( bool bRightHand ) : m_bRightHand( bRightHand ) 
{
}

void MikuModel::LoadModel( const std::wstring& model )
{
	LoadPmd( model, m_bRightHand );
}

void MikuModel::LoadMotion( const std::wstring& model )
{
	LoadVmd( model, m_bRightHand );
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

	// 
	// raw: decode with system default. 
	//      if system default is not shift-jis it will display with corrupted charactor.
	//      but, it will also happens when extract from zip archive. so, can find the file
	// unicode: decode with shift-jis
	//
	auto LoadTexture = []( Utility::ArchivePtr archive, std::string raw, std::wstring unicode, bool bNotNull, bool bSRGB )
		-> D3D11_SRV_HANDLE
	{
		//
		// zip format does not support unicode.
		// So, it is convenient to use unified ecoding, system default
		//
		auto key = archive->GetKeyName( raw );
		auto is = archive->GetFile( key );
		// Try raw name
		auto texture = TextureManager::LoadFromStream( key.generic_wstring(), *is, bSRGB );
		auto UnicodKey = archive->GetKeyName( unicode );
		// If not, try unicode interpreted name
		if (!texture->IsValid())
			texture = TextureManager::LoadFromStream( UnicodKey.generic_wstring(), *is, bSRGB );
		// If not, try default provided texture in MMD (toon01.bmp)
		auto toon = fs::path( "toon" ) / raw;
		if (!texture->IsValid())
			texture = TextureManager::LoadFromFile( toon.generic_wstring(), bSRGB );
		if (!texture->IsValid() && bNotNull)
			texture = TextureManager::LoadFromFile( "default", bSRGB );
		if (!texture->IsValid())
			return nullptr;
		return texture->GetSRV();
	};

	m_VertexBuffer.Create( pmd.m_Header.Name + L"_VertexBuf", 
		static_cast<uint32_t>(pmd.m_Vertices.size()),
		sizeof( pmd.m_Vertices[0] ),
		pmd.m_Vertices.data() );

	m_IndexBuffer.Create( pmd.m_Header.Name + L"_IndexBuf",
		static_cast<uint32_t>(pmd.m_Indices.size()),
		sizeof( pmd.m_Indices[0] ),
		pmd.m_Indices.data() );

	m_InputDesc = Pmd::InputDescriptor;

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

		if (!material.Texture.empty())
			mesh.Texture[kTextureDiffuse] = LoadTexture( archive, material.TextureRaw, material.Texture, true, true );
		if (!material.Sphere.empty())
			mesh.Texture[kTextureSphere] = LoadTexture( archive, material.SphereRaw, material.Texture, false, true );
		if (material.ToonIndex < pmd.m_ToonTextureRawList.size())
		{
			auto toonRaw = pmd.m_ToonTextureRawList[material.ToonIndex];
			auto toon = pmd.m_ToonTextureList[material.ToonIndex];
			mesh.Texture[kTextureToon] = LoadTexture( archive, toonRaw, toon, false, true );
		}
		if (mesh.Texture[kTextureSphere])
			mat.SphereOperation = material.SphereOperation;
		if (mesh.Texture[kTextureToon])
			mat.bUseToon = TRUE;

		mesh.Material = mat;

		mesh.bEdgeFlag = material.EdgeFlag > 0;

		m_Mesh.push_back(mesh);
	}

	size_t numBone = pmd.m_Bones.size();
	SetBoneNum( numBone );
	for (auto i = 0; i < numBone; i++) 
	{
		auto& boneData = pmd.m_Bones[i];

		m_Bones[i].Name = boneData.Name;
		m_BoneParent[i] = boneData.ParentBoneIndex;
		if (boneData.ParentBoneIndex < numBone)
			m_BoneChild[boneData.ParentBoneIndex].push_back( i );

		Vector3 headPos = boneData.BoneHeadPosition;
		auto parentPos = Vector3( 0.0f, 0.0f, 0.0f );

		if( boneData.ParentBoneIndex < numBone )
			parentPos = pmd.m_Bones[boneData.ParentBoneIndex].BoneHeadPosition;

		m_Bones[i].LocalPosision = headPos - parentPos;
		m_Bones[i].Scale = Vector3( 1.0f, 1.0f, 1.0f );
	}

	for (auto i = 0; i < numBone; i++)
	{
		auto& bone = m_Bones[i];
		auto& parent = m_BoneParent[i];
		auto& meshBone = m_MeshBone[i];

		meshBone.m_ParentIndex = parent;
		meshBone.m_Rotation = bone.Rotation;
		meshBone.m_LocalPosition = bone.LocalPosision;
		meshBone.m_Scale = bone.Scale;
		meshBone.m_Name = bone.Name;
		meshBone.m_MinIK = MinIK; 
		meshBone.m_MaxIK = MaxIK;

		//
		// In PMD Model minIK, maxIK is not manually given. 
		// But, bone name that contains "knee"("ひざ" in japanese) has constraint
		// that can move only in x axis and outer angle (just like human knee)
		// If this constraint is not given, then the knee looks forward 
		// in the following vmd motion. http://www.nicovideo.jp/watch/sm18737664 
		//
		if (std::string::npos != bone.Name.find( L"ひざ" )) 
		{
			if (m_bRightHand)
			{
				meshBone.m_MinIK = XMVectorSet( -XM_PI, 0, 0, 0 );
				meshBone.m_MaxIK = XMVectorSet( 0, 0, 0, 0 );
			}
			else
			{
				meshBone.m_MinIK = XMVectorSet( 0, 0, 0, 0 );
				meshBone.m_MaxIK = XMVectorSet( XM_PI, 0, 0, 0 );
			}
		}

		m_Pose[i] = Matrix4( XMMatrixMultiply( 
			XMMatrixMultiply(
				XMMatrixScalingFromVector( meshBone.m_Scale ),
				XMMatrixRotationQuaternion( meshBone.m_Rotation ) ),
			XMMatrixTranslationFromVector( meshBone.m_LocalPosition ) ) );

		if( meshBone.m_ParentIndex < numBone )
			m_Pose[i] = Matrix4( XMMatrixMultiply( m_Pose[i], m_Pose[meshBone.m_ParentIndex] ) );
		
		m_InitPose[i] = m_Pose[i];
		m_Sub[i] = Matrix4( kIdentity );
	}

	for ( auto i = 0; i < numBone; i++ )
	{
		auto& Bone = pmd.m_Bones[i];
		m_BoneIndex[Bone.Name] = i;
	}

	m_IKs = pmd.m_IKs;
}

void MikuModel::LoadVmd( const std::wstring& motionPath, bool bRightHand )
{
	using namespace std;
	using namespace Animation;
	
	std::string vmdPath( motionPath.begin(), motionPath.end() ); 

	// VMD
	m_Motion.swap(Vmd::VmdMotion::LoadFromFile( vmdPath.c_str(), bRightHand ));

	if (!m_Motion)
		return;

	for (auto& frame : m_Motion->BoneFrames)
	{
		if ( m_BoneIndex.count( frame.BoneName ) == 0)
			continue;

		KeyFrame key;
		key.Frame = frame.Frame;
		Vector3 relativeOffset = XMFLOAT3( frame.Translate );
		key.Translation = relativeOffset + m_MeshBone[m_BoneIndex[frame.BoneName]].m_LocalPosition;
		key.Rotation = Math::Quaternion( XMFLOAT4( frame.Rotation ) );

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
		char* interp = reinterpret_cast<char*>(&frame.interpolation[0]);
		float scale = 1.0f / 127.0f;

		key.BezierCoeff[kInterpX] = Vector4( interp[0], interp[8], interp[4], interp[12] ) * scale;
		key.BezierCoeff[kInterpY] = Vector4( interp[1], interp[9], interp[5], interp[13] ) * scale;
		key.BezierCoeff[kInterpZ] = Vector4( interp[2], interp[10], interp[6], interp[14] ) * scale;
		key.BezierCoeff[kInterpR] = Vector4( interp[3], interp[11], interp[7], interp[15] ) * scale;

		m_MeshBone[m_BoneIndex[frame.BoneName]].InsertKeyFrame( key );
	}

	for (auto& bone : m_MeshBone )
		bone.SortKeyFrame();
}

// Returns a quaternion such that q*start = dest
Quaternion RotationBetweenVectors(Vector3 start, Vector3 dest){
	start = Normalize( start );
	dest = Normalize( dest );
	
	Scalar cosTheta = Dot( start, dest );

	Vector3 rotationAxis;
	
	if (cosTheta < -1 + 0.001f){
		// special case when vectors in opposite directions :
		// there is no "ideal" rotation axis
		// So guess one; any will do as long as it's perpendicular to start
		// This implementation favors a rotation around the Up axis,
		// since it's often what you want to do.
		rotationAxis = Cross( Vector3( 0.0f, 0.0f, 1.0f ), start );
		if (LengthSquare( rotationAxis ) < 0.1f) // bad luck, they were parallel, try again!
			rotationAxis = Cross( Vector3( 1.0f, 0.0f, 0.0f ), start );
		
		rotationAxis = Normalize(rotationAxis);
		return Quaternion(rotationAxis, 180.0f);
	}

	// Implementation from Stan Melax's Game Programming Gems 1 article
	rotationAxis = Cross( start, dest );

	float s = sqrtf( (1.0f + cosTheta) * 2.f );
	float invs = 1 / s;

	rotationAxis *= Vector3(invs, invs, invs);
	return Quaternion( Vector4( rotationAxis, s * 0.5f ) );
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
		auto& Bone = m_MeshBone[i];
		auto Pidx = Bone.m_ParentIndex;
		Vector3 ParentPos = Vector3( kZero );
		if (Pidx < numBone) 
			ParentPos = GlobalPosition[Pidx];

		Vector3 diff = Bone.m_LocalPosition;
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
	m_VertexBuffer.Destroy();
	m_IndexBuffer.Destroy();
	m_BonePSO.Destroy();
	m_BoneIndexBuffer.Destroy();
	m_BoneVertexBuffer.Destroy();
}

void MikuModel::UpdateChildPose( int32_t idx )
{
	auto numBone = m_MeshBone.size();
	auto parentIndex = m_BoneParent[idx];

	if (parentIndex < numBone)
		m_Pose[idx] = Matrix4( XMMatrixMultiply( m_LocalPose[idx], m_Pose[parentIndex] ) );

	for (auto c : m_BoneChild[idx])
		UpdateChildPose( c );
}

void MikuModel::SetBoneNum( size_t numBones )
{
	m_BoneParent.resize( numBones );
	m_BoneChild.resize( numBones );
	m_Bones.resize( numBones );
	m_MeshBone.resize( numBones );
	m_Pose.resize( numBones );
	m_LocalPose.resize( numBones );
	m_InitPose.resize( numBones );
	m_Sub.resize( numBones );
}

void MikuModel::Update( float deltaT )
{
	static float kTime = 0.0f;
	kTime += deltaT;
	float kFrameTime = kTime * 30.0f;

	size_t numBone = m_MeshBone.size();
	for (auto i = 0; i < numBone; i++) 
		m_MeshBone[i].Interpolate( kFrameTime );
	
	for (auto i = 0; i < numBone; i++)
	{
		auto& meshBone = m_MeshBone[i];
		m_LocalPose[i] = Matrix4( XMMatrixMultiply(
			XMMatrixMultiply(
				XMMatrixScalingFromVector( meshBone.m_Scale ),
				XMMatrixRotationQuaternion( meshBone.m_Rotation ) ),
			XMMatrixTranslationFromVector( meshBone.m_LocalPosition ) ) );

		if (meshBone.m_ParentIndex < numBone)
			m_Pose[i] = Matrix4( XMMatrixMultiply( m_LocalPose[i], m_Pose[meshBone.m_ParentIndex] ) );
		else
			m_Pose[i] = m_LocalPose[i];
	}

	auto GetPosition = [&]( int32_t index ) -> Vector3
	{
		return Vector3(m_Pose[index].GetW());
	};

	//
	// Solve Constrainted IK
	// Cyclic-Coordinate-Descent（CCD）
	//
	// http://d.hatena.ne.jp/edvakf/20111102/1320268602
	// Game programming gems 3 Constrained Inverse Kinematics - Jason Weber
	//
	for (auto& ik : m_IKs)
	{
		// "effector"
		const auto ikBonePos = GetPosition( ik.IkBoneIndex );

		for (int n = 0; n < ik.IkNumIteration; n++)
		{
			// "effected" bone list in order
			for (auto c = 0; c < ik.IkLinkBondIndexList.size(); c++)
			{
				auto childIndex = ik.IkLinkBondIndexList[c];
				auto ikChildBonePos = GetPosition( childIndex );
				auto ikTargetBonePos = GetPosition( ik.IkTargetBonIndex );
				auto invLinkMtx = Invert( m_Pose[childIndex] );

				auto ikTargetVec = Vector3( invLinkMtx * ikTargetBonePos );
				auto ikBoneVec = Vector3( invLinkMtx * ikBonePos );

				auto axis = Cross( ikTargetVec, ikBoneVec );
				auto axisLen = Length( axis );
				auto sinTheta = axisLen / Length(ikTargetVec) / Length(ikBoneVec);
				if ( sinTheta < 1.0e-6f)
					continue;

				// angle to move in one iteration
				auto maxAngle = (c + 1) * ik.IkLimitedRadian * 4;
				auto theta = ASin( sinTheta );
				if (Dot( ikTargetVec, ikBoneVec) < 0.f)
					theta = XM_PI - theta;
				if (theta > maxAngle)
					theta = maxAngle;

				auto rot = Matrix4( DirectX::XMMatrixRotationAxis( axis, theta ) );
				auto LocalPose = DirectX::XMMatrixMultiply( rot, m_LocalPose[childIndex] );

				if (XMVector3NearEqual( m_MeshBone[childIndex].m_MinIK, MinIK, EpsIK ) &&
					XMVector3NearEqual( m_MeshBone[childIndex].m_MaxIK, MaxIK, EpsIK ))
				{
					m_LocalPose[childIndex] = Matrix4( LocalPose );
				}
				else
				{
					XMVECTOR scale;
					XMVECTOR rotQuat;
					XMVECTOR trans;
					DirectX::XMMatrixDecompose( &scale, &rotQuat, &trans, LocalPose );

					// 
					// http://www.euclideanspace.com/maths/geometry/rotations/euler/
					//
					auto eulerAngles = []( Quaternion q ) {
						XMFLOAT4 q1;
						DirectX::XMStoreFloat4( &q1, q );

						float test = q1.x*q1.y + q1.z*q1.w;
						float heading, attitude, bank;
						// singularity at north pole
						if (test > 0.499) { 
							heading = 2 * atan2( q1.x, q1.w );
							attitude = XM_PI / 2;
							bank = 0;
							return Vector3( bank, heading, attitude );
						}
						if (test < -0.499) { // singularity at south pole
							heading = -2 * atan2( q1.x, q1.w );
							attitude = -XM_PI / 2;
							bank = 0;
							return Vector3( bank, heading, attitude );
						}
						float sqx = q1.x*q1.x;
						float sqy = q1.y*q1.y;
						float sqz = q1.z*q1.z;
						heading = atan2( 2 * q1.y*q1.w - 2 * q1.x*q1.z, 1 - 2 * sqy - 2 * sqz );
						attitude = asin( 2 * test );
						bank = atan2( 2 * q1.x*q1.w - 2 * q1.y*q1.z, 1 - 2 * sqx - 2 * sqz );

						return Vector3( bank, heading, attitude );
					};
#if 0
					Vector3 desiredEuler = eulerAngles( Quaternion( rotQuat ) );
#if RH
					// TODO
					Vector3 clampedEuler = Clamp( desiredEuler, Vector3(0.f, 0.f, 0.f), Vector3(XM_PI, 0.f, 0.f ));
#else
#endif
					Quaternion rot = Quaternion( XMQuaternionRotationRollPitchYawFromVector( clampedEuler ) );
#else
					// cos(theta / 2)
					auto c = XMVectorGetW(rotQuat);
					Quaternion rot = Quaternion( Vector4( Sqrt( 1.0f - c*c ), 0, 0, c ) );
#endif
					m_LocalPose[childIndex] = Matrix4( XMMatrixMultiply(
						XMMatrixMultiply(
							XMMatrixScalingFromVector( scale ),
							XMMatrixRotationQuaternion( rot ) ),
						XMMatrixTranslationFromVector( trans ) ) );
				}
				UpdateChildPose( childIndex );
			}
		}
	}

	for (auto i = 0; i < numBone; i++) 
	{
		XMMATRIX mtx = XMMatrixMultiply(
			XMMatrixInverse( nullptr, m_InitPose[i] ), m_Pose[i] );
		m_Sub[i] = Matrix4( mtx );
	}
}

void MikuModel::UpdateBone( float deltaT )
{
	static float kTime = 0.0f;
	kTime += deltaT;
	float kFrameTime = kTime * 30.0f;
}

void MikuModel::Draw( GraphicsContext& gfxContext )
{
	auto elemByte = sizeof( decltype(m_Sub)::value_type );
	auto numByte = elemByte * m_Sub.size();

	gfxContext.SetVertexBuffer( 0, m_VertexBuffer.VertexBufferView() );
	gfxContext.SetIndexBuffer( m_IndexBuffer.IndexBufferView() );
	gfxContext.SetDynamicConstantBufferView( 1, numByte, m_Sub.data(), { kBindVertex } );

	for (auto mesh: m_Mesh)
	{
		gfxContext.SetDynamicDescriptors( 0, _countof( mesh.Texture ), mesh.Texture, { kBindPixel } );
		gfxContext.SetDynamicConstantBufferView( 0, sizeof(mesh.Material), &mesh.Material, { kBindPixel } );
		gfxContext.SetDynamicSampler( 0, SamplerLinearWrap, kBindPixel );
		gfxContext.SetDynamicSampler( 1, SamplerLinearClamp, kBindPixel );
		gfxContext.DrawIndexed( mesh.IndexCount, mesh.IndexOffset, 0 );
	}
}

void MikuModel::DrawBone( GraphicsContext& gfxContext )
{
	// TODO:
	// gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
	// gfxContext.SetViewportAndScissor( m_MainViewport, m_MainScissor );

	// gfxContext.SetPipelineState( m_ModelPSO );
	// gfxContext.SetDynamicConstantBufferView( 0, m_Buffer, { kBindVertex, kBindPixel } );
	gfxContext.SetPipelineState( m_BonePSO );
	gfxContext.SetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	gfxContext.SetVertexBuffer( 0, m_BoneVertexBuffer.VertexBufferView() );
	gfxContext.SetIndexBuffer( m_BoneIndexBuffer.IndexBufferView() );

	auto numBone = m_BoneAttribute.size();

	for (auto i = 0; i < numBone; i++)
	{
		XMMATRIX mat = XMMatrixMultiply( m_BoneAttribute[i], m_Sub[i] );
		gfxContext.SetDynamicConstantBufferView( 1, sizeof(mat), &mat, { kBindVertex } );
		gfxContext.DrawIndexed( m_BoneMesh.IndexCount, m_BoneMesh.IndexOffset, m_BoneMesh.VertexOffset );
	}
}