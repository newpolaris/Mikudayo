#pragma once

#pragma once

#include <array>
#include "VectorMath.h"

namespace Math
{
    using FrustumCorner = std::array<Vector3, 8>;

    class BoundingBox
    {
    public:
        BoundingBox()
        {
        }

        BoundingBox( Vector3 MinVec, Vector3 MaxVec ) : m_Min( MinVec ), m_Max( MaxVec )
        {
        }

        const Vector3& GetMin( void ) const;
        const Vector3& GetMax( void ) const;
        FrustumCorner GetCorners( void ) const;

		friend BoundingBox operator* ( const OrthogonalTransform& xform, const BoundingBox& box );	// Fast
		friend BoundingBox operator* ( const AffineTransform& xform, const BoundingBox& box );		// Slow
		friend BoundingBox operator* ( const Matrix4& xform, const BoundingBox& box );				// Slowest (and most general)

    protected:

        Vector3 m_Min;
        Vector3 m_Max;
    };

    inline FrustumCorner BoundingBox::GetCorners( void ) const
    {
        float x = m_Min.GetX(), y = m_Min.GetY(), z = m_Min.GetZ();
        float lx = m_Max.GetX(), ly = m_Max.GetY(), lz = m_Max.GetZ();

        return {
            Vector3( x, y, lz), // kNearLowerLeft
            Vector3( x, ly, lz), // kNearUpperLeft
            Vector3( lx, y, lz), // kNearLowerRight
            Vector3( lx, ly, lz), // kNearUpperRight,

            Vector3( x, y, z), // kFarLowerLeft
            Vector3( x, ly, z), // kFarUpperLeft
            Vector3( lx, y, z), // kFarLowerRight
            Vector3( lx, ly, z)  // kFarUpperRight
        };
    }

	inline const Vector3& BoundingBox::GetMin( void ) const
	{
		return m_Min;
	}

	inline const Vector3& BoundingBox::GetMax( void ) const
	{
		return m_Max;
	}

    inline BoundingBox operator* ( const OrthogonalTransform& xform, const BoundingBox& box )
    {
        Vector3 X = xform*box.GetMin();
        Vector3 Y = xform*box.GetMax();
        return BoundingBox( Min(X, Y), Max(X, Y) );
    }

    inline BoundingBox operator* ( const AffineTransform& xform, const BoundingBox& box )
    {
        Vector3 X = xform*box.GetMin();
        Vector3 Y = xform*box.GetMax();
        return BoundingBox( Min(X, Y), Max(X, Y) );
    }

    inline BoundingBox operator* ( const Matrix4& xform, const BoundingBox& box )
    {
        Vector3 X = xform.Transform(box.GetMin());
        Vector3 Y = xform.Transform(box.GetMax());
        return BoundingBox( Min(X, Y), Max(X, Y) );
    }
}