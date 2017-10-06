#pragma once

class Material
{
public:

    virtual bool IsTransparent() const;
    virtual bool IsOutline() const;
    virtual bool IsShadowCaster() const;
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
