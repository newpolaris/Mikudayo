#pragma once

#pragma once

#include <array>
#include <vector>
#include <limits>
#include "BoundingPlane.h"
#include "VectorMath.h"

namespace Math
{
    using FrustumCorner = std::array<Vector3, 8>;
    using FrustumPlanes = std::array<BoundingPlane, 6>;

    class BoundingBox
    {
    public:
        BoundingBox() : m_Min( std::numeric_limits<float>::max() ), m_Max( std::numeric_limits<float>::lowest() )
        {
        }

        BoundingBox( Vector3 MinVec, Vector3 MaxVec ) : m_Min( MinVec ), m_Max( MaxVec )
        {
        }

        BoundingBox( const std::vector<Vector3>& list ) : m_Min( std::numeric_limits<float>::max() ), m_Max( std::numeric_limits<float>::lowest() )
        {
            for (auto& vec : list)
                Merge( vec );
        }

        Vector3 GetCenter( void ) const;
        Vector3 GetExtent( void ) const;
        const Vector3& GetMin( void ) const;
        const Vector3& GetMax( void ) const;
        FrustumCorner GetCorners( void ) const;
        FrustumPlanes GetPlanes( void ) const;

        void Merge( const Vector3& vec );
        void Merge( const BoundingBox& box );
        bool Intersect( float* hitDist, const Vector3& origPt, const Vector3& dir ) const;

		friend BoundingBox operator* ( const OrthogonalTransform& xform, const BoundingBox& box );	// Fast
		friend BoundingBox operator* ( const AffineTransform& xform, const BoundingBox& box );		// Slow
		friend BoundingBox operator* ( const Matrix4& xform, const BoundingBox& box );				// Slowest (and most general)

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

    inline Vector3 BoundingBox::GetCenter() const
    {
        return (m_Min + m_Max) / 2;
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
}