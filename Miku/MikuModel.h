#pragma once

#include <string>
#include <vector>

#include "InputLayout.h"
#include "GpuBuffer.h"
#include "FileUtility.h"
#include "CommandContext.h"
#include "Vmd.h"
#include "Pmd.h"
#include "KeyFrameAnimation.h"
#include "VectorMath.h"
#include "Archive.h"

namespace Graphics
{
	using namespace Math;
	namespace fs = boost::filesystem;

	struct MaterialCB
	{
		XMFLOAT4 Diffuse;
		XMFLOAT3 Specular;
		float SpecularPower;
		XMFLOAT3 Ambient;
		int SphereOperation;
		int bUseToon;
	};

	enum ETextureType
	{
		kTextureDiffuse,
		kTextureSphere,
		kTextureToon,
		kTextureMax
	};

	struct Mesh 
	{
		MaterialCB Material;
		D3D11_SRV_HANDLE Texture[kTextureMax];
		int32_t IndexOffset;
		uint32_t IndexCount;
		bool bEdgeFlag;
	};

	struct SubmeshGeometry
	{
		uint32_t IndexCount;
		int32_t IndexOffset;
		int32_t VertexOffset;
	};

	struct Bone
	{
		std::wstring Name; 
		Vector3 Scale;
		Vector3 LocalPosision;
		Quaternion Rotation;
		Matrix4 mtxPose;
	};

	class MikuModel
	{
	public:
		MikuModel( bool bRightHand = true );
		void LoadModel( const std::wstring& model );
		void LoadMotion( const std::wstring& model );
		void LoadBone();
		void Clear();
		void Draw( GraphicsContext& gfxContext );
		void DrawBone( GraphicsContext& gfxContext );
		void Update( float dt );
		void UpdateBone( float dt );
		void UpdateChildPose( int32_t idx );
		void SetBoneNum( size_t numBones );

	private:
		void LoadPmd( const std::wstring& model, bool bRightHand );
		void LoadVmd( const std::wstring& vmd, bool bRightHand );

		void LoadPmd( Utility::ArchivePtr archive, fs::path pmdPath, bool bRightHand );

	public:
		bool m_bRightHand;
		std::vector<InputDesc> m_InputDesc;
		std::vector<Mesh> m_Mesh;
		std::vector<Bone> m_Bones;
		std::vector<Animation::BoneMotion> m_BoneMotions;
		std::vector<Matrix4> m_LocalPose;
		std::vector<Matrix4> m_Pose;
		std::vector<Matrix4> m_InitPose;
		std::vector<Matrix4> m_Sub;
		std::vector<int32_t> m_BoneParent;
		std::vector<std::vector<int32_t>> m_BoneChild;
		std::vector<Pmd::IK> m_IKs;
		std::vector<Animation::FaceMotion> m_FaceMotions;
		std::unique_ptr<Vmd::VmdMotion> m_Motion;
		std::map<std::wstring, uint32_t> m_BoneIndex;
		std::map<std::wstring, uint32_t> m_SkinIndex;
		std::vector<XMFLOAT3> m_Positions;
		std::vector<XMFLOAT3> m_MorphPositions;

		VertexBuffer m_AttributeBuffer;
		VertexBuffer m_PositionBuffer;
		IndexBuffer m_IndexBuffer;

		std::wstring m_Name;
		std::vector<XMMATRIX> m_BoneAttribute;
		SubmeshGeometry m_BoneMesh;
		VertexBuffer m_BoneVertexBuffer;
		IndexBuffer m_BoneIndexBuffer;
		GraphicsPSO m_BonePSO;
	};
}
