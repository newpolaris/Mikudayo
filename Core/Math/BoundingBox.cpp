#include "pch.h"
#include "BoundingBox.h"

using namespace Math;

inline bool NearZero(const Scalar& scalar)
{
    return std::abs( scalar ) < Scalar( g_XMEpsilon.v );
}

namespace Math {
    BoundingBox operator* ( const Matrix4& xform, const BoundingBox& box )
    {
        BoundingBox b;
        FrustumCorner corners = box.GetCorners();
        for (auto& point : corners)
            b.Merge( Vector3( xform * point ) );
        return b;
    }
}

void BoundingBox::Merge( const Vector3& vec )
{
    m_Min = Min( m_Min, vec );
    m_Max = Max( m_Max, vec );
}

// Use code from NVIDIA PSM DEMO
bool BoundingBox::Intersect( float* hitDist, const Vector3& origPt, const Vector3& dir ) const
{
    ASSERT( hitDist != nullptr );

    *hitDist = 0.f; // safe initial value
    Vector3 hitPt = origPt;

    Vector4 sides[6] = {
        Vector4( 1, 0, 0, -m_Min.GetX() ), Vector4( -1, 0, 0, m_Max.GetX() ),
        Vector4( 0, 1, 0, -m_Min.GetY() ), Vector4(  0,-1, 0, m_Max.GetY() ),
        Vector4( 0, 0, 1, -m_Min.GetZ() ), Vector4(  0, 0,-1, m_Max.GetZ() )
    };

    bool inside = false;

    for ( int i=0; (i<6) && !inside; i++ )
    {
        Scalar cosTheta = Scalar(DirectX::XMPlaneDotNormal( sides[i], dir ));
        Scalar dist = Scalar(DirectX::XMPlaneDotCoord( sides[i], origPt ));

        //  if we're nearly intersecting, just punt and call it an intersection
        if ( NearZero(dist)) return true;
        //  skip nearly (&actually) parallel rays
        if ( NearZero(cosTheta) ) continue;
        //  only interested in intersections along the ray, not before it.
        *hitDist = -dist / cosTheta;
        if (*hitDist < 0.f) continue;
        hitPt = (*hitDist)*dir + origPt;

        inside = true;

        for ( int j=0; (j<6) && inside; j++ )
        {
            if ( j==i )
                continue;
            Scalar d = Scalar(DirectX::XMPlaneDotCoord( sides[j], hitPt ));
            inside = ((d + 0.00015f) >= 0.f);
        }
    }

    return inside;
}

FrustumPlanes BoundingBox::GetPlanes( void ) const
{
    return {
        BoundingPlane( 0.0f,  1.0f,  0.0f, -m_Min.GetY() ), // bottom
        BoundingPlane( 0.0f, -1.0f,  0.0f, m_Max.GetY() ), // top
        BoundingPlane( 1.0f,  0.0f,  0.0f, -m_Min.GetX() ), // left
        BoundingPlane(-1.0f,  0.0f,  0.0f, m_Max.GetX() ), // right
    #if RIGHT
        BoundingPlane( 0.0f,  0.0f,  1.0f, -m_Min.GetZ() ), // back
        BoundingPlane( 0.0f,  0.0f, -1.0f, m_Max.GetZ() ), // front
    #else
        BoundingPlane( 0.0f,  0.0f, -1.0f, m_Max.GetZ() ), // back
        BoundingPlane( 0.0f,  0.0f,  1.0f, -m_Min.GetZ() ), // front
    #endif
    };
}