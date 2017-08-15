#include "stdafx.h"
#include "../Common.h"

#include <numeric>

#include "VectorMath.h"
#include "Camera.h"
#include "Math/BoundingSphere.h"

using namespace Math;

TEST(BoundingPlaneTest, GetCenter)
{
    BoundingPlane Plane( Normalize(Vector3(0, 10, 0)), 15.f );
    Vector3 Center( kZero );
    Scalar Dist = Plane.DistanceFromPoint( Center );
    EXPECT_NEAR(Dist, Scalar(15.f), 1e-5f);
    Dist = Plane.DistanceFromPoint( Vector3(0, 20, 0));
    EXPECT_NEAR(Dist, Scalar(35.f), 1e-5f);
    Dist = Plane.DistanceFromPoint( Vector3(0, -20, 0));
    EXPECT_NEAR(Dist, Scalar(-5.f), 1e-5f);
}
