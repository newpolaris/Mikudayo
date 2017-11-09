#pragma once

#include <array>
#include "BoundingPlane.h"
#include "BoundingSphere.h"
#include "BoundingBox.h"

namespace Math
{
    using FrustumCorner = std::array<Vector3, 8>;
    class BoundingFrustum
    {
    public:
        BoundingFrustum() {}
        BoundingFrustum( const Matrix4& Matrix );

        FrustumCorner GetFrustumCorners( void ) const;

        bool IntersectBox( BoundingBox box );

		Vector3 m_FrustumCorners[8];		// the corners of the frustum
        BoundingPlane m_FrustumPlanes[6]; // the bounding planes (toward inside)
    };

    inline FrustumCorner BoundingFrustum::GetFrustumCorners( void ) const
    {
        FrustumCorner Corners;
        std::copy( m_FrustumCorners, m_FrustumCorners + 8, Corners.begin() );
        return Corners;
    }

}
