#include "pch.h"
#include "BoundingBox.h"

using namespace Math;

inline bool NearZero(const Scalar& scalar)
{
    return std::abs( scalar ) < Scalar( DirectX::g_XMEpsilon.v );
}

namespace Math {
    BoundingBox operator* ( const Matrix4& xform, const BoundingBox& box )
    {
        Vector3 X = xform.Transform(box.GetMin());
        Vector3 Y = xform.Transform(box.GetMax());
        BoundingBox k( Min(X, Y), Max(X, Y) );

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

void BoundingBox::Merge( const BoundingBox& box )
{
    m_Min = Min( box.GetMin(), m_Min );
    m_Max = Max( box.GetMax(), m_Max );
}

// Use code from NVIDIA PSM DEMO
bool BoundingBox::Intersect( float* hitDist, const Vector3& origPt, const Vector3& dir ) const
{
    ASSERT( hitDist != nullptr );

    Vector3 hitPt = origPt;
    FrustumPlanes sides = GetPlanes();
    bool inside = false;
    float hitD = 0.f;
    for (int i = 0; i < 6; i++)
    {
        Scalar cosTheta = Scalar(DirectX::XMPlaneDotNormal( sides[i], dir ));
        Scalar dist = Scalar(DirectX::XMPlaneDotCoord( sides[i], origPt ));

        //  if we're nearly intersecting, just punt and call it an intersection
        if (std::abs(dist) < 0.00015f) {
        // if (NearZero(dist)) {
		    hitD = 0.f;
            inside = true;
            break;
        }
        //  skip nearly (&actually) parallel rays
        // if (NearZero( cosTheta )) 
        if (std::abs(cosTheta) < 0.00015f) 
            continue;
        //  only interested in intersections along the ray, not before it.
        hitD = -dist / cosTheta;
        if (hitD < 0.f) 
            continue;
        hitPt = hitD * dir + origPt;
        inside = true;
        for (int j = 0; j < 6; j++)
        {
            if (j == i)
                continue;
            Scalar d = Scalar( DirectX::XMPlaneDotCoord( sides[j], hitPt ) );
            inside = ((d + 0.0015f) >= 0.f);
            if (!inside) break;
        }
        if (inside) break;
    }
    if (inside)
        *hitDist = hitD;
    return inside;
}

FrustumPlanes BoundingBox::GetPlanes( void ) const
{
    return {
        BoundingPlane( 0.0f,  1.0f,  0.0f, -m_Min.GetY() ), // bottom
        BoundingPlane( 0.0f, -1.0f,  0.0f, m_Max.GetY() ), // top
        BoundingPlane( 1.0f,  0.0f,  0.0f, -m_Min.GetX() ), // left
        BoundingPlane(-1.0f,  0.0f,  0.0f, m_Max.GetX() ), // right
    #define RIGHT 1
    #if RIGHT
        BoundingPlane( 0.0f,  0.0f,  1.0f, -m_Min.GetZ() ), // back
        BoundingPlane( 0.0f,  0.0f, -1.0f, m_Max.GetZ() ), // front
    #else
        BoundingPlane( 0.0f,  0.0f, -1.0f, m_Max.GetZ() ), // back
        BoundingPlane( 0.0f,  0.0f,  1.0f, -m_Min.GetZ() ), // front
    #endif
    };
}