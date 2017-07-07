#pragma once

#include <string>
#include <vector>

#include "InputLayout.h"
#include "GpuBuffer.h"
#include "FileUtility.h"
#include "CommandContext.h"
#include "Vmd.h"
#include "Pmd.h"
#include "VectorMath.h"
#include "Archive.h"
#include "KeyFrameAnimation.h"

class ManagedTexture;

namespace Graphics
{
	using namespace Math;
	namespace fs = boost::filesystem;

	enum eObjectFilter { kOpaque = 0x1, kCutout = 0x2, kTransparent = 0x4, kAll = 0xF, kNone = 0x0 };

	__declspec(align(16)) struct MaterialCB
	{
		XMFLOAT4 Diffuse;
		XMFLOAT3 Specular;
		float SpecularPower;
		XMFLOAT3 Ambient;
		int SphereOperation;
		int bUseTexture;
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
		bool isTransparent() const { return Material.Diffuse.w < 1.f; }
        bool LoadTexture( GraphicsContext& gfxContext );

		MaterialCB Material;
        const ManagedTexture* Texture[kTextureMax];		int32_t IndexOffset;
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
		Vector3 Translate;
	};

	class MikuModel
	{
	public:
		MikuModel( bool bRightHand = true );
		~MikuModel();
		void Clear();
		void Draw( GraphicsContext& gfxContext, eObjectFilter Filter );
		void DrawBone( GraphicsContext& gfxContext );
        void Load();
		void SetModel( const std::wstring& model );
		void SetMotion( const std::wstring& model );
        void SetPosition( Vector3 postion );
		void Update( float kFrameTime );

	private:
        void LoadBone();
		void LoadPmd( const std::wstring& model, bool bRightHand );
		void LoadPmd( Utility::ArchivePtr archive, fs::path pmdPath, bool bRightHand );
		void LoadVmd( const std::wstring& vmd, bool bRightHand );

		void SetBoneNum( size_t numBones );
        void LoadBoneMotion( const std::vector<Vmd::BoneFrame>& frames ); 

		void UpdateIK( const Pmd::IK& ik );
		void UpdateChildPose( int32_t idx );

	public:
		bool m_bRightHand;
        std::wstring m_ModelPath;
        std::wstring m_MotionPath;
		std::vector<Mesh> m_Mesh;
		std::vector<Bone> m_Bones;
		std::vector<Pmd::IK> m_IKs;
		std::vector<OrthogonalTransform> m_toRoot; // inverse inital pose ( inverse Rest)
		std::vector<OrthogonalTransform> m_LocalPose; // offset matrix
		std::vector<OrthogonalTransform> m_Pose; // cumulative transfrom matrix from root
		std::vector<OrthogonalTransform> m_Skinning; // final skinning transform
		std::vector<DualQuaternion> m_SkinningDual; // final skinning transform
		std::vector<int32_t> m_BoneParent; // parent index
		std::vector<std::vector<int32_t>> m_BoneChild; // child indices
		std::map<std::wstring, uint32_t> m_BoneIndex;
		std::map<std::wstring, uint32_t> m_MorphIndex;
        std::vector<Vector3> m_MorphDelta; // tempolar space to store morphed position delta
		std::vector<Animation::BoneMotion> m_BoneMotions;
        enum { kMorphBase = 0 };
		std::vector<Animation::MorphMotion> m_MorphMotions;
		Animation::CameraMotion m_CameraMotion;

		std::vector<XMFLOAT3> m_VertexPos; // original vertex position
		std::vector<XMFLOAT3> m_VertexMorphedPos; // temporal vertex positions which affected by face animation

		VertexBuffer m_AttributeBuffer;
		VertexBuffer m_PositionBuffer;
		IndexBuffer m_IndexBuffer;

        Matrix4 m_ModelTrnasform;
		std::wstring m_Name;
		std::vector<XMMATRIX> m_BoneAttribute;
		SubmeshGeometry m_BoneMesh;
		VertexBuffer m_BoneVertexBuffer;
		IndexBuffer m_BoneIndexBuffer;
		GraphicsPSO m_BonePSO;
	};

    inline void MikuModel::SetPosition( Vector3 postion )
    {
        m_ModelTrnasform = Matrix4::MakeTranslate(postion);
    }
}
