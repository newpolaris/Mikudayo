#include "stdafx.h"
#include "ModelAccessory.h"
#include "Material.h"

struct DefaultMaterial : IMaterial
{
    Math::Vector4 diffuse;
    Math::Vector3 specular;
    Math::Vector3 ambient;
    Math::Vector3 emissive;
    float specularPower; // specular exponent
    enum { kDiffuse, kTexCount = 1 };
    const ManagedTexture* textures[kTexCount];
    std::wstring name;

    bool IsTransparent() const override;
    void Bind( GraphicsContext& gfxContext ) override;
    RenderPipelinePtr GetPipeline( RenderQueue Queue ) override;
};

bool DefaultMaterial::IsTransparent() const
{
    bool bHasTransparentTexture = textures[kDiffuse] && textures[kDiffuse]->IsTransparent();
    return diffuse.GetW() < 1.0f || bHasTransparentTexture;
}

void DefaultMaterial::Bind( GraphicsContext& gfxContext )
{
    D3D11_SRV_HANDLE SRV[kTexCount] = { nullptr };
    for (auto i = 0; i < _countof( textures ); i++)
    {
        if (textures[i] == nullptr) continue;
        SRV[i] = textures[i]->GetSRV();
    }
    gfxContext.SetDynamicDescriptors( 1, _countof( SRV ), SRV, { kBindPixel } );

    __declspec(align(16)) struct {
        Vector4 diffuse;
        Vector3 specular;
        Vector3 ambient;
        Vector3 emissive;
        float specularPower; // specular exponent
        uint32_t bTexture[kTexCount];
    } material = {
        diffuse, specular, ambient, emissive, specularPower
    };
    for (auto i = 0; i < _countof( textures ); i++)
        material.bTexture[i] = (SRV[i] == nullptr ? 0 : 1);
    gfxContext.SetDynamicConstantBufferView( 4, sizeof( material ), &material, { kBindVertex, kBindPixel } );
}

RenderPipelinePtr DefaultMaterial::GetPipeline( RenderQueue Queue )
{
    return nullptr;
}

void ModelAccessory::LoadMaterials()
{
    for (uint32_t i = 0; i < m_Header.materialCount; i++)
    {
        std::shared_ptr<DefaultMaterial> material = std::make_shared<DefaultMaterial>();
        const Material& pMaterial = m_pMaterial[i];
        Vector4 diffuse = Vector4( pMaterial.diffuse, pMaterial.opacity );
        material->ambient = pMaterial.ambient;
        material->diffuse = diffuse;
        material->specular = pMaterial.specular;
        material->emissive = pMaterial.emissive;
        material->specularPower = pMaterial.shininess;
        const ManagedTexture** MatTextures = material->textures;
        MatTextures[0] = LoadTexture(pMaterial.texDiffusePath, true);
        m_Materials.push_back( std::move( material ) );
    }
}
