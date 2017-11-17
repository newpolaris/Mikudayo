#include "stdafx.h"
#include "BaseMaterial.h"
#include "BaseModel.h"

using namespace Math;

BaseMaterial::BaseMaterial() :
    diffuse(Scalar(1.f)),
    specular(kZero),
    ambient(kZero),
    emissive(kZero),
    transparent(kZero),
    opacity(1.f),
    shininess(0.f),
    specularStrength(0.f),
    shader(L"Default")
{
    for (auto i = 0; i < kTexCount; i++)
        textures[i] = nullptr;
}

bool BaseMaterial::IsTransparent() const
{
    bool bHasTransparentTexture = textures[kDiffuse] && textures[kDiffuse]->IsTransparent();
    return opacity < 1.0f || bHasTransparentTexture;
}

void BaseMaterial::Bind( GraphicsContext& gfxContext )
{
    D3D11_SRV_HANDLE SRV[kTexCount] = { nullptr };
    for (auto i = 0; i < _countof( textures ); i++)
    {
        if (textures[i] == nullptr) continue;
        SRV[i] = textures[i]->GetSRV();
    }
    gfxContext.SetDynamicDescriptors( 1, _countof( SRV ), SRV, { kBindPixel } );

    __declspec(align(16)) struct {
        Vector3 diffuse;
        Vector3 specular;
        Vector3 ambient;
        Vector3 emissive;
        Vector3 transparent; // light passing through a transparent surface is multiplied by this filter color
        float opacity;
        float shininess; // specular exponent
        float specularStrength; // multiplier on top of specular color
        uint32_t bTexture[kTexCount];
    } material = {
        diffuse, specular, ambient, emissive, transparent, opacity, shininess, specularStrength
    };
    for (auto i = 0; i < _countof( textures ); i++)
        material.bTexture[i] = (SRV[i] == nullptr ? 0 : 1);
    gfxContext.SetDynamicConstantBufferView( 4, sizeof( material ), &material, { kBindVertex, kBindPixel } );
}

RenderPipelinePtr BaseMaterial::GetPipeline( RenderQueue Queue ) 
{
    const RenderPipelineList& list = BaseModel::FindTechniques( shader );
    return list[Queue];
}