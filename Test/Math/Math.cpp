#include "../Common.h"

#include "VectorMath.h"
#include "Camera.h"
#include "OrthographicCamera.h"

#include "Math/BoundingBox.h"

using namespace Math;

TEST(MathTest, OrthograhicMatrix)
{
    float W = 800.f, H = 600.f, N = 1.f, F = 1000.f;
    Matrix4 A = OrthographicMatrix( W, H, N, F, false );
    Matrix4 B = Matrix4(DirectX::XMMatrixOrthographicRH( W, H, N, F ));

    EXPECT_TRUE(Near(A, B, Scalar( 1e-5f )));
}

TEST(MathTest, OrthographicOffCenterMatrix)
{
    float L = -400.f, R = 500.f, B = - 300.f, T = 400.f, N = 1.f, F = 1000.f;
    Matrix4 A = OrthographicMatrix( L, R, B, T, N, F, false );
    Matrix4 C = Matrix4(DirectX::XMMatrixOrthographicOffCenterRH( L, R, B, T, N, F ));

    EXPECT_TRUE(Near(A, C, Scalar( 1e-6f )));
}

TEST(MathTest, ClipToViewport)
{
    const float W = 1920, H = 1024;
    AffineTransform T = AffineTransform(Matrix3::MakeScale( 0.5f, 0.5f, 1.0f ), Vector3(0.5f, 0.5f, 0.0f));
    T = AffineTransform::MakeScale( Vector3(W, H, 1.0f) ) * T;
    Vector3 A = T * Vector3( -1.f, -1.f, 0.f );
    EXPECT_THAT( A.GetX(), MatcherNearFast( 1e-5f, Scalar(0.f) ) );
    EXPECT_THAT( A.GetY(), MatcherNearFast( 1e-5f, Scalar(0.f) ) );
    Vector3 B = T * Vector3( 1.f, 1.f, 0.f );
    EXPECT_THAT( B.GetX(), MatcherNearFast( 1e-5f, Scalar(W) ) );
    EXPECT_THAT( B.GetY(), MatcherNearFast( 1e-5f, Scalar(H) ) );
}

TEST(BoundingBoxTest, Corner)
{
    float Left = -1.f, Right = 1.f, Bottom = -1.f, Top = 1.f, Near = -0.1f, Far = -10000.f;
    Vector3 Corners[] =
    {
        Vector3( Left, Bottom, Near ),	// Near lower left
        Vector3( Left, Top, Near ),	    // Near upper left
        Vector3( Right, Bottom, Near),	// Near lower right
        Vector3( Right, Top, Near ),	// Near upper right
        Vector3( Left, Bottom, Far ),	// Far lower left
        Vector3( Left, Top, Far ),	    // Far upper left
        Vector3( Right, Bottom, Far ),	// Far lower right
        Vector3( Right, Top, Far ),	    // Far upper right
    };

    BoundingBox box( Vector3(Left, Bottom, Far), Vector3(Right, Top, Near));
    EXPECT_THAT( Corners, Pointwise( MatcherNearFast( FLT_EPSILON ), box.GetCorners() ) );
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

TEST(FrustumTest, WorldSpaceCorner)
{
    Vector3 Pos( 10.f, -10.f, 10.f ), At( 0.f, 0.f, 5.f ), Up( kYUnitVector );

    Camera cam;
    cam.SetEyeAtUp( Pos, At, Up );
    cam.Update();

    Frustum FrustumVS( cam.GetProjMatrix() );
    Frustum FrustumWS = cam.GetCameraToWorld() * FrustumVS;
    auto Coners = cam.GetWorldSpaceFrustum().GetFrustumCorners();

    EXPECT_THAT( FrustumWS.GetFrustumCorners(), Pointwise( MatcherNearRelative( FLT_EPSILON ), Coners) );
}

TEST(FrustumTest, WorldSpaceCorner2)
{
    Vector3 Pos( 0.f ), At( 1.f, 1.f, 1.f ), Up( kYUnitVector );

    OrthographicCamera cam;
    cam.SetEyeAtUp( Pos, Normalize(At), Up );
    cam.Update();

    Frustum FrustumVS( cam.GetProjMatrix() );
    Frustum FrustumWS = cam.GetCameraToWorld() * FrustumVS;
    auto Coners = cam.GetWorldSpaceFrustum().GetFrustumCorners();

    EXPECT_THAT( FrustumWS.GetFrustumCorners(), Pointwise( MatcherNearRelative( FLT_EPSILON ), Coners) );

    Matrix4 InvView = Invert(cam.GetViewMatrix());
    FrustumCorner CornersWS;
    auto CornersVS = FrustumVS.GetFrustumCorners();
    for (auto i = 0; i < CornersVS.size(); i++)
        CornersWS[i] = InvView.Transform( CornersVS[i] );

    EXPECT_THAT( CornersWS, Pointwise( MatcherNearRelative( 1e-5f), Coners) );
}

TEST(CameraTest, LookAt)
{
    Vector3 Pos( 10.f, -10.f, 10.f ), At( 0.f, 0.f, 5.f ), Up( kYUnitVector );

    Camera cam;
    cam.SetEyeAtUp( Pos, At, Up );
    auto& CameraToWorld = cam.GetCameraToWorld();
    auto View = Matrix4(~CameraToWorld);
    Matrix4 RefView = Matrix4( XMMatrixLookAtRH( Pos, At, Up ) );

    EXPECT_THAT( View, MatcherNearFast( 1e-5f, RefView ) );
}