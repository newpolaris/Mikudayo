#pragma once

#include "Material.h"

class Visitor;

struct BaseMaterial : IMaterial
{
    Math::Vector3 diffuse;
    Math::Vector3 specular;
    Math::Vector3 ambient;
    Math::Vector3 emissive;
    Math::Vector3 transparent; // light passing through a transparent surface is multiplied by this filter color
    float opacity;
    float shininess; // specular exponent
    float specularStrength; // multiplier on top of specular color
    enum { kDiffuse, kSpecular, kEmissive, kNormal, kLightmap, kReflection, kTexCount = 6 };
    const ManagedTexture* textures[kTexCount];
    std::wstring name;

    bool IsTransparent() const override;
    void Bind( GraphicsContext& gfxContext ) override;
    RenderPipelinePtr GetPipeline( RenderQueue Queue ) override;
};

