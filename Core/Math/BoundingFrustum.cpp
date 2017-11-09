#include "pch.h"
#include "BoundingFrustum.h"

namespace Math
{
    // NVIDIA's PracticalPSM
    bool PlaneIntersection( Vector3& Point, const Vector4& p0, const Vector4& p1, const Vector4& p2 )
    {
        Vector3 n0(p0), n1(p1), n2(p2);
        Vector3 n1_n2 = Cross( n1, n2 );
        Vector3 n2_n0 = Cross( n2, n0 );
        Vector3 n0_n1 = Cross( n0, n1 );

        float cosTheta = Dot( n0, n1_n2 );
        if (std::abs( cosTheta ) < FLT_EPSILON)
            return false;

        float secTheta = 1.f / cosTheta;

        n1_n2 = n1_n2 * p0.GetW();
        n2_n0 = n2_n0 * p1.GetW();
        n0_n1 = n0_n1 * p2.GetW();

        Point = -(n1_n2 + n2_n0 + n0_n1) * secTheta;
        return true;
    }
}

using namespace Math;

// http://www.cs.otago.ac.nz/postgrads/alexis/planeExtraction.pdf
BoundingFrustum::BoundingFrustum( const Matrix4& Matrix )
{
    auto _11 = Matrix.GetX().GetX(), _12 = Matrix.GetX().GetY(), _13 = Matrix.GetX().GetZ(), _14 = Matrix.GetX().GetW(),
        _21 = Matrix.GetY().GetX(), _22 = Matrix.GetY().GetY(), _23 = Matrix.GetY().GetZ(), _24 = Matrix.GetY().GetW(),
        _31 = Matrix.GetZ().GetX(), _32 = Matrix.GetZ().GetY(), _33 = Matrix.GetZ().GetZ(), _34 = Matrix.GetZ().GetW(),
        _41 = Matrix.GetW().GetX(), _42 = Matrix.GetW().GetY(), _43 = Matrix.GetW().GetZ(), _44 = Matrix.GetW().GetW();

    Vector4 column1( _11, _21, _31, _41 ), column2( _12, _22, _32, _42 ), column3( _13, _23, _33, _43 ), column4( _14, _24, _34, _44 );
    Vector4 plane[6];
    plane[0] = column4 + column1; // left
    plane[1] = column4 - column1;
    plane[2] = column4 + column2; // bottom
    plane[3] = column4 - column2;
#if OPENGL
    plane[4] = column4 + column3;
#else
    plane[4] = column3;
#endif
    plane[5] = column4 - column3; // far
    for (uint32_t i = 0; i < 6; i++)
        m_FrustumPlanes[i] = BoundingPlane( Vector4( DirectX::XMPlaneNormalize( plane[i] ) ) );

    for (int i = 0; i < 8; i++)  // compute extrema
    {
    #if !NORMALIZE_PLANE
        const Vector4& p0 = (i & 1) ? plane[4] : plane[5];
        const Vector4& p1 = (i & 2) ? plane[3] : plane[2];
        const Vector4& p2 = (i & 4) ? plane[0] : plane[1];
    #else
        const Vector4& p0 = (i & 1) ? m_FrustumPlanes[4] : m_FrustumPlanes[5];
        const Vector4& p1 = (i & 2) ? m_FrustumPlanes[3] : m_FrustumPlanes[2];
        const Vector4& p2 = (i & 4) ? m_FrustumPlanes[0] : m_FrustumPlanes[1];
    #endif

        ASSERT( PlaneIntersection( m_FrustumCorners[i], p0, p1, p2 ) );
    }
}

// NVIDIA's PracticalPSM
bool BoundingFrustum::IntersectBox( BoundingBox box )
{
    bool intersect = false;
    const Vector3 minpt = box.m_Min, maxpt = box.m_Max;
    for (int i = 0; i < 6; i++)
    {
        Vector3 normal = m_FrustumPlanes[i].GetNormal();
        bool vx = normal.GetX() < 0.f, vy = normal.GetY() < 0.f, vz = normal.GetZ() < 0.f;
        // pVertex is diagonally opposed to nVertex
        Vector3 nVertex( vx ? minpt.GetX() : maxpt.GetX(), vy ? minpt.GetY() : maxpt.GetY(), vz ? minpt.GetZ() : maxpt.GetZ() );
        Vector3 pVertex( vx ? maxpt.GetX() : minpt.GetX(), vy ? maxpt.GetY() : minpt.GetY(), vz ? maxpt.GetY() : minpt.GetZ() );
        const Vector3 zero( kZero );
        if (XMVector3Less( DirectX::XMPlaneDotCoord( Vector4( m_FrustumPlanes[i] ), nVertex ), zero ))
            return false;
        if (XMVector3Less( DirectX::XMPlaneDotCoord( Vector4( m_FrustumPlanes[i] ), pVertex ), zero ))
            intersect = true;
    }
    return true;
}
