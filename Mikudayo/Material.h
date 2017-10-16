#pragma once

#include "RenderType.h"

class Material
{
public:

    virtual bool IsTransparent() const;
    virtual bool IsOutline() const;
    virtual bool IsShadowCaster() const;
    virtual RenderPipelinePtr GetPipeline( RenderQueue Queue );
};

inline bool Material::IsTransparent() const
{
    return false; 
}

inline bool Material::IsOutline() const
{
    return false;
}

inline bool Material::IsShadowCaster() const
{
    return false;
}

inline RenderPipelinePtr Material::GetPipeline( RenderQueue )
{
    return nullptr;
}
