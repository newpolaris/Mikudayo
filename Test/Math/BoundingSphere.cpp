#include "stdafx.h"
#include "../Common.h"

#include <random>
#include <numeric>

#include "VectorMath.h"
#include "Camera.h"
#include "Math/BoundingSphere.h"

using namespace Math;

TEST(BoundingSphereTest, GetCenter)
{
    Vector3 Center( 5.f, 5.f, 5.f );
    BoundingSphere sphere( Vector3( 5.f, 5.f, 5.f ), Scalar( 10.f ) );
    EXPECT_THAT( sphere.GetCenter(), MatcherNearFast( 1e-5f, Center ) );
}

TEST(BoundingSphereTest, Matrix4)
{
    BoundingSphere sphere( Vector3( 0.f, 0.f, 0.f ), Scalar( 10.f ) );
    Matrix4 scale = Matrix4::MakeScale( Vector3(1.f, 2.f, 3.f) );
    BoundingSphere result = scale * sphere;

    EXPECT_THAT( result.GetCenter(), MatcherNearFast( 1e-5f, Vector3( 0.f, 0.f, 0.f ) ) );
    EXPECT_NEAR( result.GetRadius(), 30.f, 0.f );
}

TEST(BoundingSphereTest, AffineTransform)
{
    BoundingSphere sphere( Vector3( 0.f, 0.f, 0.f ), Scalar( 10.f ) );
    AffineTransform scale = AffineTransform::MakeScale( Vector3(1.f, 2.f, 3.f) );
    BoundingSphere result = scale * sphere;

    EXPECT_THAT( result.GetCenter(), MatcherNearFast( 1e-5f, Vector3( 0.f, 0.f, 0.f ) ) );
    EXPECT_NEAR( result.GetRadius(), 30.f, 0.f );
}

TEST(BoundingSphereTest, ComputeBoundingSphereFromVertices)
{
    std::vector<uint16_t> indices = { 0, 1 };
    std::vector<XMFLOAT3> vertices = {
        XMFLOAT3( 0.f, 0.f, 0.f ),
        XMFLOAT3( 1.f, 1.f, 1.f ),
    };
    Scalar R = Length(Vector3( 0.5f ) - Vector3( 1.f ));
    BoundingSphere sphere = ComputeBoundingSphereFromVertices( vertices, indices, 2, 0 );
    EXPECT_NEAR( sphere.GetRadius(), R, 0.f );
    Vector3 Center = sphere.GetCenter();
    EXPECT_THAT(Center, MatcherNearFast( 1e-5f, Vector3( 0.5f, 0.5f, 0.5f ) ) );
}

TEST(BoundingSphereTest, ComputeBoundingSphereFromVertices2)
{
    std::vector<XMFLOAT3> vertices = {
	    { -0.5f, -0.707107f, -0.5f },
	    { -0.653282f, -0.382683f, -0.653282f },
	    { -0.270598f, -0.707107f, -0.653282f },
	    { -0.653282f, -0.707107f, -0.270598f },
	    { -0.853553f, -0.382683f, -0.353553f },
	    { -0.353553f, -0.382683f, -0.853553f },
	    { -0.270598f, -0.92388f, -0.270598f },
	    { -0.353553f, -0.92388f, -0.146447f },
	    { -0.146447f, -0.92388f, -0.353553f },
	    { 0.0f, -0.707107f, -0.707107f },
	    { -0.707107f, 0.0f, -0.707107f },
	    { -0.707107f, -0.707107f, -0.0f },
	    { 0.0f, -0.707107f, -0.707107f },
	    { -0.92388f, -0.382683f, -0.0f },
	    { -0.382683f, -0.92388f, -0.0f },
	    { 0.0f, -0.92388f, -0.382683f },
	    { 0.0f, -0.382683f, -0.92388f },
	    { -0.92388f, 0.0f, -0.382683f },
	    { -0.382683f, 0.0f, -0.92388f },
	    { 0.0f, -0.92388f, -0.382683f },
	    { 0.0f, -0.382683f, -0.92388f },
	    { 0.146447f, -0.92388f, -0.353553f },
	    { -0.353553f, -0.92388f, 0.146447f },
	    { -0.653282f, -0.707107f, 0.270598f },
	    { 0.270598f, -0.707107f, -0.653282f },
	    { 0.0f, 0.0f, -1.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { -1.0f, 0.0f, -0.0f },
	    { 0.0f, 0.0f, -1.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { 0.0f, -1.0f, -0.0f },
	    { -0.653282f, 0.382683f, -0.653282f },
	    { -0.270598f, -0.92388f, 0.270598f },
	    { 0.270598f, -0.92388f, -0.270598f },
	    { 0.353553f, -0.382683f, -0.853553f },
	    { -0.853553f, -0.382683f, 0.353553f },
	    { -0.353553f, 0.382683f, -0.853553f },
	    { -0.853553f, 0.382683f, -0.353553f },
	    { -0.146447f, -0.92388f, 0.353553f },
	    { 0.353553f, -0.92388f, -0.146447f },
	    { -0.5f, -0.707107f, 0.5f },
	    { 0.5f, -0.707107f, -0.5f },
	    { 0.0f, 0.382683f, -0.92388f },
	    { 0.0f, -0.92388f, 0.382683f },
	    { 0.0f, 0.382683f, -0.92388f },
	    { 0.382683f, 0.0f, -0.92388f },
	    { 0.382683f, -0.92388f, -0.0f },
	    { -0.92388f, 0.0f, 0.382683f },
	    { -0.92388f, 0.382683f, -0.0f },
	    { 0.146447f, -0.92388f, 0.353553f },
	    { 0.353553f, -0.92388f, 0.146447f },
	    { -0.653282f, -0.382683f, 0.653282f },
	    { 0.653282f, -0.382683f, -0.653282f },
	    { 0.270598f, -0.92388f, 0.270598f },
	    { -0.270598f, -0.707107f, 0.653282f },
	    { 0.653282f, -0.707107f, -0.270598f },
	    { -0.5f, 0.707107f, -0.5f },
	    { -0.653282f, 0.707107f, -0.270598f },
	    { -0.270598f, 0.707107f, -0.653282f },
	    { -0.853553f, 0.382683f, 0.353553f },
	    { 0.353553f, 0.382683f, -0.853553f },
	    { 0.0f, -0.707107f, 0.707107f },
	    { 0.707107f, 0.0f, -0.707107f },
	    { -0.707107f, 0.707107f, -0.0f },
	    { 0.0f, 0.707107f, -0.707107f },
	    { 0.0f, 0.707107f, -0.707107f },
	    { -0.707107f, 0.0f, 0.707107f },
	    { 0.707107f, -0.707107f, -0.0f },
	    { -0.353553f, -0.382683f, 0.853553f },
	    { 0.853553f, -0.382683f, -0.353553f },
	    { 0.270598f, -0.707107f, 0.653282f },
	    { 0.653282f, -0.707107f, 0.270598f },
	    { 0.5f, -0.707107f, 0.5f },
	    { -0.653282f, 0.707107f, 0.270598f },
	    { 0.270598f, 0.707107f, -0.653282f },
	    { -0.270598f, 0.92388f, -0.270598f },
	    { -0.653282f, 0.382683f, 0.653282f },
	    { 0.653282f, 0.382683f, -0.653282f },
	    { -0.353553f, 0.92388f, -0.146447f },
	    { -0.146447f, 0.92388f, -0.353553f },
	    { -0.382683f, 0.0f, 0.92388f },
	    { 0.0f, -0.382683f, 0.92388f },
	    { -0.382683f, 0.92388f, -0.0f },
	    { 0.92388f, -0.382683f, -0.0f },
	    { 0.0f, 0.92388f, -0.382683f },
	    { 0.92388f, 0.0f, -0.382683f },
	    { 0.0f, 0.92388f, -0.382683f },
	    { -0.5f, 0.707107f, 0.5f },
	    { 0.5f, 0.707107f, -0.5f },
	    { -0.353553f, 0.92388f, 0.146447f },
	    { 0.146447f, 0.92388f, -0.353553f },
	    { 0.353553f, -0.382683f, 0.853553f },
	    { 0.853553f, -0.382683f, 0.353553f },
	    { 0.853553f, 0.382683f, -0.353553f },
	    { -0.353553f, 0.382683f, 0.853553f },
	    { 0.270598f, 0.92388f, -0.270598f },
	    { -0.270598f, 0.92388f, 0.270598f },
	    { 0.653282f, -0.382683f, 0.653282f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 1.0f, 0.0f, -0.0f },
	    { 0.0f, 0.0f, 1.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { 0.0f, 1.0f, -0.0f },
	    { -0.270598f, 0.707107f, 0.653282f },
	    { 0.653282f, 0.707107f, -0.270598f },
	    { -0.146447f, 0.92388f, 0.353553f },
	    { 0.353553f, 0.92388f, -0.146447f },
	    { 0.382683f, 0.0f, 0.92388f },
	    { 0.0f, 0.382683f, 0.92388f },
	    { 0.92388f, 0.0f, 0.382683f },
	    { 0.92388f, 0.382683f, -0.0f },
	    { 0.0f, 0.92388f, 0.382683f },
	    { 0.382683f, 0.92388f, -0.0f },
	    { 0.707107f, 0.707107f, -0.0f },
	    { 0.707107f, 0.0f, 0.707107f },
	    { 0.0f, 0.707107f, 0.707107f },
	    { 0.353553f, 0.92388f, 0.146447f },
	    { 0.146447f, 0.92388f, 0.353553f },
	    { 0.270598f, 0.92388f, 0.270598f },
	    { 0.353553f, 0.382683f, 0.853553f },
	    { 0.853553f, 0.382683f, 0.353553f },
	    { 0.653282f, 0.707107f, 0.270598f },
	    { 0.270598f, 0.707107f, 0.653282f },
	    { 0.653282f, 0.382683f, 0.653282f },
	    { 0.5f, 0.707107f, 0.5f },
    };
    std::vector<uint16_t> indices(vertices.size());
    std::iota(indices.begin(), indices.end(), 0);

    BoundingSphere sphere = ComputeBoundingSphereFromVertices( vertices, indices, (uint32_t)indices.size(), 0 );
    Scalar RSquare = sphere.GetRadius()*sphere.GetRadius();
    for (auto& pt : vertices)
    {
        Scalar Dist = LengthSquare( sphere.GetCenter() - pt );
        EXPECT_LE( float( Dist ), float( RSquare )+FLT_EPSILON );
    }
}
