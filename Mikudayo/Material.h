#pragma once

#include "RenderType.h"

class IMaterial
{
public:

    virtual bool IsTransparent() const;
    virtual bool IsOutline() const;
    virtual bool IsShadowCaster() const;
    virtual RenderPipelinePtr GetPipeline( RenderQueue Queue );
};

inline bool IMaterial::IsTransparent() const
{
    return false; 
}

inline bool IMaterial::IsOutline() const
{
    return false;
}

inline bool IMaterial::IsShadowCaster() const
{
    return false;
}

inline RenderPipelinePtr IMaterial::GetPipeline( RenderQueue )
{
    return nullptr;
}
