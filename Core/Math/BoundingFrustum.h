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

		// Test whether the bounding sphere intersects the frustum.  Intersection is defined as either being
		// fully contained in the frustum, or by intersecting one or more of the planes.
        bool IntersectBox( const BoundingBox& box ) const;
		bool IntersectSphere( const BoundingSphere& sphere ) const;

		Vector3 m_FrustumCorners[8];		// the corners of the frustum
        BoundingPlane m_FrustumPlanes[6]; // the bounding planes (toward inside)
    };

    inline FrustumCorner BoundingFrustum::GetFrustumCorners( void ) const
    {
        FrustumCorner Corners;
        std::copy( m_FrustumCorners, m_FrustumCorners + 8, Corners.begin() );
        return Corners;
    }

	inline BoundingFrustum operator* ( const OrthogonalTransform& xform, const BoundingFrustum& Frustum )
	{
        BoundingFrustum result;

        for (int i = 0; i < 8; ++i)
            result.m_FrustumCorners[i] = xform * Frustum.m_FrustumCorners[i];

        for (int i = 0; i < 6; ++i)
            result.m_FrustumPlanes[i] = xform * Frustum.m_FrustumPlanes[i];

        return result;
	}

	inline BoundingFrustum operator* ( const AffineTransform& xform, const BoundingFrustum& Frustum )
	{
		BoundingFrustum result;

		for (int i = 0; i < 8; ++i)
			result.m_FrustumCorners[i] = xform * Frustum.m_FrustumCorners[i];

		Matrix4 XForm = Transpose(Invert(Matrix4(xform)));

		for (int i = 0; i < 6; ++i)
			result.m_FrustumPlanes[i] = BoundingPlane(XForm * Vector4(Frustum.m_FrustumPlanes[i]));

		return result;
	}

	inline BoundingFrustum operator* ( const Matrix4& mtx, const BoundingFrustum& Frustum )
	{
		BoundingFrustum result;
		for (int i = 0; i < 8; ++i)
			result.m_FrustumCorners[i] = Vector3( mtx * Frustum.m_FrustumCorners[i] );
		Matrix4 XForm = Transpose(Invert(mtx));
		for (int i = 0; i < 6; ++i)
			result.m_FrustumPlanes[i] = BoundingPlane(XForm * Vector4(Frustum.m_FrustumPlanes[i]));
		return result;
	}
}
