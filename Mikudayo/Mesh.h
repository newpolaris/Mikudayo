#pragma once

namespace Math
{
    class Frustum;
}

class IMesh
{
public:
    virtual bool IsIntersect( const Math::Frustum& frustumWS ) const;
};

inline bool IMesh::IsIntersect( const Math::Frustum& frustumWS ) const
{
    (frustumWS);
    return true;
}