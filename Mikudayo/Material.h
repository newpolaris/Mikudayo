#pragma once

class Material
{
public:

    virtual bool IsTransparent() const;
};

inline bool Material::IsTransparent() const
{
    return false; 
}
