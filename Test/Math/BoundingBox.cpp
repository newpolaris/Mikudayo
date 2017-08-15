#include "stdafx.h"
#include "../Common.h"

#include <random>

#include "VectorMath.h"
#include "Camera.h"
#include "Math/BoundingBox.h"

using namespace Math;

TEST(BoundingBoxTest, OrthogonalTransform)
{
    BoundingBox box( Vector3( -5.f ), Vector3( 5.f ) );
    BoundingBox result = OrthogonalTransform::MakeTranslation( Vector3( 5.f ) ) * box;
    EXPECT_THAT( result.GetMin(), MatcherNearFast( 1e-5f, Vector3( 0.f ) ) );

    result = Matrix4::MakeScale( Vector3( 2.f ) ) * box;
    EXPECT_THAT( result.GetMin(), MatcherNearFast( 1e-5f, Vector3( -10.f ) ) );
}

TEST(BoundingBoxTest, FrustumCorner)
{
    float Left = -1.f, Right = 1.f, Bottom = -1.f, Top = 1.f, Near = 0.1f, Far = 10000.f;
    Vector3 min( Left, Bottom, Near ), max( Right, Top, Far );

    Matrix4 Proj = OrthographicMatrix( Left, Right, Bottom, Top, Near, Far, false );
    Frustum frustum( Proj );
    BoundingBox box( min, max );
    auto corners = box.GetCorners();

    EXPECT_THAT( frustum.GetFrustumCorners(), Pointwise( MatcherNearRelative( FLT_EPSILON ), corners) );
}

