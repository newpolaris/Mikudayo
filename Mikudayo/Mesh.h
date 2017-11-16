#pragma once

namespace Math
{
    class BoundingFrustum;
}

class IMesh
{
public:
    virtual bool IsIntersect( const Math::BoundingFrustum& frustumWS ) const;
};

inline bool IMesh::IsIntersect( const Math::BoundingFrustum& frustumWS ) const
{
    (frustumWS);
    return true;
}