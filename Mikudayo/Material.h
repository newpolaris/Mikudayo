#pragma once

class Material
{
public:

    virtual bool IsTransparent() const;
    virtual bool IsOutline() const;
};

inline bool Material::IsTransparent() const
{
    return false; 
}

inline bool Material::IsOutline() const
{
    return false;
}