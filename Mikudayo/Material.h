#pragma once

#include "RenderType.h"

class IMaterial
{
public:

    virtual bool IsOutline() const;
    virtual bool IsShadowCaster() const;
    virtual bool IsTransparent() const;
    virtual bool IsTwoSided() const;
    virtual void Bind( GraphicsContext& gfxContext );
    virtual RenderPipelinePtr GetPipeline( RenderQueue Queue );
};

inline bool IMaterial::IsOutline() const
{
    return false;
}

inline bool IMaterial::IsShadowCaster() const
{
    return false;
}

inline bool IMaterial::IsTransparent() const
{
    return false; 
}

inline bool IMaterial::IsTwoSided() const
{
    return false;
}

inline void IMaterial::Bind( GraphicsContext& )
{
}

inline RenderPipelinePtr IMaterial::GetPipeline( RenderQueue )
{
    return nullptr;
}
