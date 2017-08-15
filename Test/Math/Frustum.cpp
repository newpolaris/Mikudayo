#include "stdafx.h"
#include "../Common.h"

#include "Math/BoundingBox.h"
#include "Math/Frustum.h"
#include "Camera.h"
#include "OrthographicCamera.h"

using namespace Math;

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

TEST(FrustumTest, IntersectSphere)
{
    OrthographicCamera cam;
    cam.SetOrthographic(-1, 1, -1, 1, -1, 1);
    cam.Update();

    const Frustum& FrustumVS = cam.GetViewSpaceFrustum();
    BoundingSphere Sphere(Vector3(kZero), 1.f);
    EXPECT_TRUE(FrustumVS.IntersectSphere(Sphere));
    Sphere = BoundingSphere(Vector3(0, 1.f, 0), 1.f);
    EXPECT_TRUE(FrustumVS.IntersectSphere(Sphere));
    Sphere = BoundingSphere(Vector3(0, -1.f, 0), 1.f);
    EXPECT_TRUE(FrustumVS.IntersectSphere(Sphere));
    Sphere = BoundingSphere(Vector3(0.3f, 0.3f, -0.3f), 0.3f);
    EXPECT_TRUE(FrustumVS.IntersectSphere(Sphere));
    Sphere = BoundingSphere(Vector3(0, 2.5, 0), 1.f);
    EXPECT_FALSE(FrustumVS.IntersectSphere(Sphere));
    Sphere = BoundingSphere(Vector3(2.5, 0, 0), 1.f);
    EXPECT_FALSE(FrustumVS.IntersectSphere(Sphere));
    Sphere = BoundingSphere(Vector3(0, 0, 2.5), 1.f);
    EXPECT_FALSE(FrustumVS.IntersectSphere(Sphere));
    Sphere = BoundingSphere(Vector3(0, 0, -2.5), 1.f);
    EXPECT_FALSE(FrustumVS.IntersectSphere(Sphere));
}
