#include "stdafx.h"
#include "../Common.h"

#include <iostream>

#include "Math/Matrix3.h"
#include "Bullet/LinearMath.h"
#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btTransform.h"

using namespace Math;

TEST(LinearMathTest, btVector3)
{
    btVector3 a( 5, 10, 100 );
    Math::Vector3 b = Convert( a );

    EXPECT_NEAR(b.GetX(), a[0], 1e-5f);
    EXPECT_NEAR(b.GetY(), a[1], 1e-5f);
    EXPECT_NEAR(b.GetZ(), a[2], 1e-5f);
}

TEST(LinearMathTest, btVector4)
{
    btVector4 a( 5, 10, 100, 1000 );
    Math::Vector4 b = Convert( a );

    EXPECT_NEAR(b.GetX(), a[0], 1e-5f);
    EXPECT_NEAR(b.GetY(), a[1], 1e-5f);
    EXPECT_NEAR(b.GetZ(), a[2], 1e-5f);
    EXPECT_NEAR(b.GetW(), a[3], 1e-5f);
}

TEST(LinearMathTest, btQuaternion)
{
}

TEST(LinearMathTest, btTransform)
{
#define POS 100.f, 10.f, 1.f
    Math::AffineTransform Transform( Math::Quaternion(), Math::Vector3(POS) );
    auto a = Transform * Math::Vector3( POS );

    btTransform btTrans( btQuaternion(0, 0, 0, 1), btVector3(POS));
    auto b = btTrans * btVector3( POS );

    // Binary compatible, if no rotation related
    auto btTrans2 = Convert( Transform );
    auto c = btTrans2 * btVector3( POS );

    auto Transform2 = Convert( btTrans );
    auto d = Transform2 * Math::Vector3( POS );
#undef POS

    EXPECT_NEAR(a.GetX(), b[0], 1e-5f);
    EXPECT_NEAR(a.GetY(), b[1], 1e-5f);
    EXPECT_NEAR(a.GetZ(), b[2], 1e-5f);

    EXPECT_NEAR(a.GetX(), c[0], 1e-5f);
    EXPECT_NEAR(a.GetY(), c[1], 1e-5f);
    EXPECT_NEAR(a.GetZ(), c[2], 1e-5f);

    EXPECT_NEAR(d.GetX(), b[0], 1e-5f);
    EXPECT_NEAR(d.GetY(), b[1], 1e-5f);
    EXPECT_NEAR(d.GetZ(), b[2], 1e-5f);
}

TEST(LinearMathTest, btTransform2)
{
    const float Angle = 3.14f/6;
    Math::Vector3 Axis( Math::kZUnitVector );
    Math::AffineTransform Trans(Math::Quaternion( Axis, Angle ));
    btTransform btTrans( btQuaternion( Convert( Axis ), Angle ) );
    Math::AffineTransform convertedTrans = Convert(btTrans);

    EXPECT_THAT( convertedTrans.GetBasis().GetX(), MatcherNearFast( 1e-5f, Trans.GetBasis().GetX() ) );
    EXPECT_THAT( convertedTrans.GetBasis().GetY(), MatcherNearFast( 1e-5f, Trans.GetBasis().GetY() ) );
    EXPECT_THAT( convertedTrans.GetBasis().GetZ(), MatcherNearFast( 1e-5f, Trans.GetBasis().GetZ() ) );
}
