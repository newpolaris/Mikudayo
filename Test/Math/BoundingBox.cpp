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