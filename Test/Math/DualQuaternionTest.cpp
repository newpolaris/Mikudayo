#include "stdafx.h"
#include "../Common.h"

#include <random>

#include "VectorMath.h"
#include "Camera.h"
#include "Math/DualQuaternion.h"
#include "GLMMath.h"

using namespace Math;

TEST(DualQuaternionTest, Identity)
{
    DualQuaternion identity;
    EXPECT_THAT( identity.Real, MatcherNearFast( 1e-5f, Quaternion( 0, 0, 0, 1 ) ) );
    EXPECT_THAT( identity.Dual, MatcherNearFast( 1e-5f, Quaternion( 0, 0, 0, 0 ) ) );
}

void LinearBlendTestFixutre( Quaternion q0, Vector3 t0, Quaternion q1, Vector3 t1 )
{
    // ContructFromOrhogonal
    OrthogonalTransform O0( q0, t0 );
    OrthogonalTransform O1( q1, t1);

    DualQuaternion dq0( O0 );
    glm::dualquat dq1( glm::Convert( O0.GetRotation() ), glm::Convert( O0.GetTranslation() ) );
    EXPECT_THAT( dq0.Real, MatcherNearFast( 1e-5f, glm::Convert(dq1.real) ) );
    EXPECT_THAT( dq0.Dual, MatcherNearFast( 1e-5f, glm::Convert(dq1.dual) ) );
    DualQuaternion dq2( O1 );
    glm::dualquat dq3( glm::Convert( O1.GetRotation() ), glm::Convert( O1.GetTranslation() ) );
    EXPECT_THAT( dq2.Real, MatcherNearFast( 1e-5f, glm::Convert(dq3.real) ) );
    EXPECT_THAT( dq2.Dual, MatcherNearFast( 1e-5f, glm::Convert(dq3.dual) ) );

    // Normalize
    dq0 = Normalize( dq0 );
    dq1 = glm::normalize( dq1 );
    EXPECT_THAT( dq0, MatcherNearFast( 1e-5f, glm::Convert(dq1) ) );
    dq2 = Normalize( dq2 );
    dq3 = glm::normalize( dq3 );
    EXPECT_THAT( dq2, MatcherNearFast( 1e-5f, glm::Convert(dq3) ) );

    float weight0 = 0.479398996f;
    float weight1 = 1 - weight0;
    if (glm::dot( dq1.real, dq3.real ) < 0)
        weight1 = -weight1;

    auto blend0 = dq1 * weight0 + dq3 * weight1;
    auto blend1 = dq0 * weight0 + dq2 * weight1;

    blend0 = glm::normalize( blend0 );
    blend1 = Normalize( blend1 );

    Vector3 pos( 0.171990007, 12.1642303, 1.71758997 );

    auto post0 = blend0 * glm::Convert( pos );
    auto post1 = blend1.Transform( pos );

    EXPECT_THAT( post1, MatcherNearFast( 1e-5f, glm::Convert(post0) ) );
}

TEST(DualQuaternionTest, LinearBlend)
{
    Quaternion r0;
    Vector3 t0( kZero );
    Quaternion r1( -0.00954649132, -4.84010876e-08, -8.60519975e-08, 0.999954462 );
    Vector3 t1( -2.54038900e-06, -0.0190820694, 0.292559505 );

    LinearBlendTestFixutre( r0, t0, r1, t1 );

    Quaternion r2( -0.0116940839, -0.311019421, -0.0152804749, 0.950208783 );
    Vector3 t2( 0.508194447, 1.34565258, 1.93716824 );

    Quaternion r3( 0.462718546, -0.369088709, -0.197909713, 0.781342983 );
    Vector3 t3( 1.64743185, 8.63631821, -8.05393887 );

    LinearBlendTestFixutre( r2, t2, r3, t3 );
}

TEST(DualQuaternionTest, Basic)
{
    DualQuaternion a;
    DualQuaternion b = a + Scalar( 5 ) * Scalar( 2 );
    EXPECT_THAT( b.Real, MatcherNearFast( 1e-5f, Quaternion( 10, 10, 10, 11 ) ) );
    EXPECT_THAT( b.Dual, MatcherNearFast( 1e-5f, Quaternion( 10, 10, 10, 10 ) ) );
}

TEST(DualQuaternionTest, BasicOrder)
{
    OrthogonalTransform o1( Quaternion( 1, 2, 3, 4 ), Vector3( 5, 6, 7 ) );
    OrthogonalTransform o2( Quaternion( 1, 2, 3, 4 ), Vector3( 5, 6, 7 ) );
    DualQuaternion dq0(o1);
    DualQuaternion dq1(o1);
    float weight0 = 0.3f, weight1 = 0.7f;
    DualQuaternion a = dq0 * weight0;
    DualQuaternion b = dq1 * weight1;
    DualQuaternion c = a + b;
    EXPECT_THAT( dq0 * weight0 + dq1 * (1 - weight0), MatcherNearFast( 1e-5f, c ) );
}

TEST(DualQuaternionTest, Translate)
{
    // Vector3 tt = (Vector3(Dual*Real.GetW()) - Vector3(Real)*Dual.GetW() + Cross(Vector3(Real), Vector3(Dual))) * 2.f;
}

TEST(DualQuaternionTest, Transform)
{
    Vector3 Vec( 0, 0, 1 );
    OrthogonalTransform O( Quaternion( XM_PI/4, 0, 0 ), Vector3( 10, 0, 0 ) );
    Vector3 Result = O * Vec;
    DualQuaternion DQ(O);
    EXPECT_THAT( DQ.Transform(Vec), MatcherNearFast( 1e-5f, Result ) );
}
