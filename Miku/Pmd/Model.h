#pragma once

#include <string>
#include <vector>

#include "GpuBuffer.h"
#include "Vmd.h"
#include "Pmd.h"
#include "IModel.h"
#include "KeyFrameAnimation.h"
#include "Math/BoundingSphere.h"
#include "Math/BoundingBox.h"

class ManagedTexture;

namespace Graphics {
namespace Pmd {
    using namespace ::Pmd;
	using namespace Math;
    using namespace Utility;

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
        const ManagedTexture* Texture[kTextureMax];
        int32_t IndexOffset;
		uint32_t IndexCount;
        BoundingSphere Sphere;
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

	class Model : public IModel
	{
	public:

		Model( bool bRightHand = true );
		~Model();
        void Clear( void );
        void Draw( GraphicsContext& gfxContext, eObjectFilter Filter ) override;
        BoundingSphere GetBoundingSphere();
        BoundingBox GetBoundingBox() override;
        bool LoadModel( ArchivePtr& Archive, Path& FilePath ) override;
		bool LoadMotion( const std::wstring& motion ) override;
        void SetModel( const std::wstring& model );
        void SetMotion( const std::wstring& model );
        void SetPosition( Vector3 postion );
        void SetBoundingSphere( void );
        void SetBoundingBox( void );
		void Update( float kFrameTime ) override;

	private:

		void DrawBone( void );
		void DrawBoundingSphere( void );
        void LoadBoneMotion( const std::vector<Vmd::BoneFrame>& frames );
		void SetBoneNum( size_t numBones );
        void SetVisualizeSkeleton();
		void UpdateChildPose( int32_t idx );
		void UpdateIK( const IK& ik );

	public:
		bool m_bRightHand;
        std::wstring m_ModelPath;
        std::wstring m_MotionPath;
		std::vector<Mesh> m_Mesh;
		std::vector<Bone> m_Bones;
		std::vector<IK> m_IKs;
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
        std::vector<uint16_t> m_Indices;

		VertexBuffer m_AttributeBuffer;
		VertexBuffer m_PositionBuffer;
		IndexBuffer m_IndexBuffer;

        Matrix4 m_ModelTransform;
		std::wstring m_Name;

        uint32_t m_RootBoneIndex; // named as center
        BoundingSphere m_BoundingSphere;
        BoundingBox m_BoundingBox;

        std::vector<AffineTransform> m_BoneAttribute;
	};

    inline void Model::SetPosition( Vector3 postion )
    {
        m_ModelTransform = Matrix4::MakeTranslate(postion);
    }
} // namespace Pmx
} // namespace Graphics
