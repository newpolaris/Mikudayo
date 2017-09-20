#pragma once

#include <string>
#include <vector>
#include "Math/BoundingBox.h"
#include "Model.h"

using namespace Math;

namespace Pmx {
    struct Bone;
}
struct CustomShaderInfo;
struct SoftBodySetting;
struct FxTechnique;
class PmxModel : public Model
{
public:

    struct VertexAttribute
    {
        XMFLOAT3 Normal;
        XMFLOAT2 UV;
        uint32_t BoneID[4] = { 0, };
        float    Weight[4] = { 0.f };
        float    EdgeSize = 0.f;
    };

    enum ETextureType
    {
        kTextureDiffuse,
        kTextureSphere,
        kTextureToon,
        kTextureDefaultCount,
    };

    struct TexturePath
    {
        bool bSRGB;
        std::wstring Path;
    };

	__declspec(align(16))
    struct MaterialCB
	{
		XMFLOAT4 Diffuse;
		XMFLOAT3 Specular;
		float SpecularPower;
		XMFLOAT3 Ambient;
		int32_t SphereOperation;
		int32_t bUseTexture;
		int32_t bUseToon;
        float EdgeSize;
        Color EdgeColor;
	};

    struct Material
    {
        std::wstring Name;
        MaterialCB CB;
        float EdgeSize;
        Color EdgeColor;
        std::vector<TexturePath> TexturePathes;
        std::vector<const ManagedTexture*> Textures;
        std::shared_ptr<FxTechnique> m_TechniqueColor;
        std::shared_ptr<FxTechnique> m_TechniqueShadow;
        bool SetTexture( GraphicsContext& gfxContext ) const;
    };

	struct Mesh
	{
        uint32_t MaterialIndex;
        int32_t IndexOffset;
		uint32_t IndexCount;
        BoundingSphere BoundSphere;
	};

    struct Bone
    {
        std::wstring Name;
        Vector3 Translate; // Offset from parent
        Vector3 Position;
        int32_t DestinationIndex;
        Vector3 DestinationOffset;
        int32_t Parent;
        std::vector<int32_t> Child;
    };

    std::wstring m_Name;
    std::string m_SoftBodyName;
    std::wstring m_TextureRoot;
    std::vector<XMFLOAT3> m_VertexPosition;
    std::vector<VertexAttribute> m_VertexAttribute;
    std::vector<uint32_t> m_Indices;
    std::vector<Material> m_Materials;
    std::vector<Mesh> m_Mesh;

    // Bone
    uint32_t m_RootBoneIndex; // model center
    std::vector<Bone> m_Bones;
    std::vector<AffineTransform> m_Pose;
    std::vector<AffineTransform> m_LocalPose; // offset matrix

    std::map<std::wstring, uint32_t> m_MaterialIndex;
    std::map<std::wstring, uint32_t> m_BoneIndex;

    IndexBuffer m_IndexBuffer;

    PmxModel();
    virtual ~PmxModel();
    void Clear();
    bool Load( const ModelInfo& Info );

protected:

    bool GenerateResource( void );
    bool LoadFromFile( const std::wstring& FilePath );
    const ManagedTexture* LoadTexture( std::wstring ImageName, bool bSRGB );
    bool SetCustomShader( const CustomShaderInfo& Data );
    bool SetPhysicsBody( const std::string& SoftBodyName );
};