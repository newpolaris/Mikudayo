#pragma once

#include <string>
#include <vector>
#include "Math/BoundingBox.h"
#include "IModel.h"
#include "Mesh.h"
#include "Material.h"
#include "Pmx.h"
#include "RenderPass.h"

using namespace Math;

namespace Pmx {
    extern std::vector<InputDesc> VertElem;
}

class PmxModel : public IModel
{
public:

    enum ETextureType
    {
        kTextureDiffuse,
        kTextureSphere,
        kTextureToon,
        kTextureMax
    };

    struct TexturePath
    {
        bool bSRGB;
        std::wstring Path;
    };

    struct MaterialCB
	{
		XMFLOAT4 Diffuse;
		XMFLOAT3 Specular;
		float SpecularPower;
		XMFLOAT3 Ambient;
        float EdgeSize;
        XMFLOAT4 EdgeColor;
        Color MaterialToon;
		int32_t SphereOperation;
		int32_t bUseTexture;
		int32_t bUseToon;
        int32_t PAD32;
	};

    struct Material : public IMaterial
    {
        std::wstring Name;
        std::wstring ShaderName;
        MaterialCB CB;
        bool bOutline = false;
        bool bCastShadowMap = false;
        bool bTwoSided = false;
        RenderPipelineList Techniques;
        std::vector<TexturePath> TexturePathes;
        const ManagedTexture* Textures[kTextureMax];
        bool IsOutline() const override;
        bool IsShadowCaster() const override;
        bool IsTransparent() const override;
        bool IsTwoSided() const override;
        RenderPipelinePtr GetPipeline( RenderQueue Queue ) override;
        void SetTexture( GraphicsContext& gfxContext ) const;
    };

	struct Mesh : public IMesh
	{
        uint32_t MaterialIndex;
        int32_t IndexOffset;
		uint32_t IndexCount;
        Math::BoundingSphere BoundSphere;
	};
	
	struct Bone
    {
        std::wstring Name;
        Vector3 Translate; // Offset from parent
        Vector3 Position;
        int32_t DestinationIndex;
        Vector3 DestinationOffset;
        bool bInherentRotation = false;
        bool bInherentTranslation = false;
        int32_t ParentInherentBoneIndex = -1;
        float ParentInherentBoneCoefficent = 0.f;
		int32_t Parent;
        std::vector<int32_t> Child;
	};

    struct IKChild
    {
        int32_t BoneIndex;
        uint8_t bLimit;
        XMFLOAT3 MinLimit;
        XMFLOAT3 MaxLimit;
    };

    struct IKAttr
    {
        int32_t BoneIndex;
        int32_t TargetBoneIndex;
        int32_t NumIteration;
        float LimitedRadian;
        std::vector<IKChild> Link;
    };

    struct SkinTypeUnit
    {
        uint32_t Type;
        Pmx::SkinUnit Unit;
    };

    std::wstring m_Name;
    std::wstring m_TextureRoot;
    std::wstring m_DefaultShader;
    std::vector<uint32_t> m_Indices;
    std::vector<Material> m_Materials;
    std::vector<Mesh> m_Mesh;

    // Bone
    uint32_t m_RootBoneIndex; // model center
    std::vector<Bone> m_Bones;
    std::vector<IKAttr> m_IKs;

    // RigidBody
    std::vector<Pmx::RigidBody> m_RigidBodies;
    std::vector<Pmx::Joint> m_Joints;

    std::map<std::wstring, uint32_t> m_MaterialIndex;
    std::map<std::wstring, uint32_t> m_BoneIndex;
    std::vector<XMFLOAT3> m_Position;
    std::vector<XMFLOAT3> m_Normal;
    std::vector<XMFLOAT2> m_TextureCoord;
    std::vector<SkinTypeUnit> m_SkinningUnit;
    std::vector<float> m_EdgeScale;

    IndexBuffer m_IndexBuffer;
    ByteAddressBuffer m_SkinningUnitbuffer;
    BoundingBox m_BoundingBox;

    static void Initialize();
    static void Shutdown();

    PmxModel();

    void Clear() override;
    bool Load( const ModelInfo& Info ) override;

protected:

    std::wstring GetImagePath( const std::wstring& FilePath );
    Color GetMaterialToon( const std::wstring& FilePath );
    bool GenerateResource( void );
    bool LoadFromFile( const std::wstring& FilePath );
    const ManagedTexture* LoadTexture( std::wstring ImageName, bool bSRGB );
    bool SetBoundingBox();
    bool SetCustomShader( const CustomShaderInfo& Data );
    bool SetDefaultShader( const std::wstring& Name );
};