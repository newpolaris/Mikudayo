#pragma once

#include "Model.h"
#include "Material.h"
#include "Mesh.h"
#include "SceneNode.h"

typedef std::shared_ptr<struct BaseMaterial> MaterialPtr;
typedef std::shared_ptr<struct BaseMesh> MeshPtr;
typedef std::shared_ptr<class BaseModel> ModelPtr;

typedef std::vector<MaterialPtr> Materials;
typedef std::vector<MeshPtr> Meshes;

struct BaseMaterial : IMaterial
{
    Vector3 diffuse;
    Vector3 specular;
    Vector3 ambient;
    Vector3 emissive;
    Vector3 transparent; // light passing through a transparent surface is multiplied by this filter color
    float opacity;
    float shininess; // specular exponent
    float specularStrength; // multiplier on top of specular color
    enum { kDiffuse, kSpecular, kEmissive, kNormal, kLightmap, kReflection, kTexCount = 6 };
    const ManagedTexture* textures[kTexCount];
    std::wstring name;

    bool IsTransparent() const override;
    RenderPipelinePtr GetPipeline( RenderQueue Queue ) override;

    void Accept( Visitor& visitor );
    void Bind( GraphicsContext& gfxContext );
};

struct BaseMesh : IMesh
{
    BoundingBox boundingBox;
    MaterialPtr material;

    unsigned int materialIndex;
    unsigned int attribsEnabled;
    unsigned int attribsEnabledDepth;
    unsigned int vertexStride;
    unsigned int vertexStrideDepth;
    // Attrib attrib[maxAttribs];
    // Attrib attribDepth[maxAttribs];
    uint32_t startIndex;
    uint32_t baseVertex;
    unsigned int vertexDataByteOffset;
    unsigned int vertexCount;
    unsigned int indexDataByteOffset;
    unsigned int indexCount;

    unsigned int vertexDataByteOffsetDepth;
    unsigned int vertexCountDepth;
};

class BaseModel : public IModel, public SceneNode
{
public:

    static void Initialize();
    static void Shutdown();

    BaseModel();

    virtual void Clear() override;
    virtual bool Load( const ModelInfo& info ) override;
    virtual void Render( GraphicsContext& gfxContext, Visitor& visitor ) override;

protected:

    std::wstring m_FileName;
    std::wstring m_DefaultShader;

    Materials m_Materials;
    Meshes m_Meshes;

    VertexBuffer m_VertexBuffer;
    IndexBuffer m_IndexBuffer;
    VertexBuffer m_VertexBufferDepth;
    IndexBuffer m_IndexBufferDepth;
};