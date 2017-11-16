#include "stdafx.h"
#include "../Common.h"

#include "Camera.h"
#include "OrthographicCamera.h"
#include "Math/BoundingBox.h"
#include "Math/Frustum.h"
#include "Math/BoundingFrustum.h"

using namespace Math;

TEST(FrustumTest, WorldSpaceCorner)
{
    Vector3 Pos( 10.f, -10.f, 10.f ), At( 0.f, 0.f, 5.f ), Up( kYUnitVector );

    Camera cam;
    cam.SetEyeAtUp( Pos, At, Up );
    cam.Update();

    Math::BoundingFrustum FrustumVS( cam.GetProjMatrix() );
    Math::BoundingFrustum FrustumWS = cam.GetCameraToWorld() * FrustumVS;
    auto Coners = cam.GetWorldSpaceFrustum().GetFrustumCorners();

    EXPECT_THAT( FrustumWS.GetFrustumCorners(), Pointwise( MatcherNearRelative( FLT_EPSILON ), Coners) );
}

TEST(FrustumTest, WorldSpaceCorner2)
{
    Vector3 Pos( 0.f ), At( 1.f, 1.f, 1.f ), Up( kYUnitVector );

    OrthographicCamera cam;
    cam.SetEyeAtUp( Pos, Normalize(At), Up );
    cam.Update();

    Math::BoundingFrustum FrustumVS( cam.GetProjMatrix() );
    Math::BoundingFrustum FrustumWS = cam.GetCameraToWorld() * FrustumVS;
    auto Coners = cam.GetWorldSpaceFrustum().GetFrustumCorners();

    EXPECT_THAT( FrustumWS.GetFrustumCorners(), Pointwise( MatcherNearRelative( FLT_EPSILON ), Coners) );

    Matrix4 InvView = Invert(cam.GetViewMatrix());
    FrustumCorner CornersWS;
    auto CornersVS = FrustumVS.GetFrustumCorners();
    for (auto i = 0; i < CornersVS.size(); i++)
        CornersWS[i] = InvView.Transform( CornersVS[i] );

    EXPECT_THAT( CornersWS, Pointwise( MatcherNearRelative( 1e-5f), Coners) );
}

#if 0
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
#endif

#include <DirectXCollision.h>

TEST(FrustumTest, FromProjMatrix)
{
    DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveRH( 100, 50, 1, 100 );
    Math::Matrix4 mat( proj );
    Math::Frustum frustum( mat );
    DirectX::BoundingFrustum ref( proj );
    Math::FrustumCorner corner = frustum.GetFrustumCorners();
    std::vector<DirectX::XMFLOAT3> vCorners(8);
    ref.GetCorners(vCorners.data());
    DirectX::XMVECTOR planes[6];
    ref.GetPlanes( &planes[0], &planes[1], &planes[2], &planes[3], &planes[4], &planes[5] );
    std::vector<Math::Vector3> vv;
    std::copy( vCorners.begin(), vCorners.end(), std::back_inserter( vv ) );
}

TEST(FrustumTest, FromViewProjMatrix)
{
    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtRH( Math::Vector3( 0, 10, 10 ), Math::Vector3( Math::kZero ), Math::Vector3( Math::kYUnitVector ) );
    DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveRH( 100, 50, 1, 100 );
    Math::Matrix4 mat( proj );
    Math::Frustum projFrustum( mat );
    Math::Frustum viewProjFrustum( Math::Matrix4( proj )*Math::Matrix4( view ) );
    Math::Frustum converted =  Math::Invert( Math::Matrix4( view ) ) * projFrustum;
    // Result: Not equal
}

TEST(FrustumTest, FromProjMatrixLH)
{
    DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveLH( 100, 50, 1, 100 );
    Math::Matrix4 mat( proj );
    Math::Frustum frustum( mat );
    DirectX::BoundingFrustum ref( proj );
    Math::FrustumCorner corner = frustum.GetFrustumCorners();
    std::vector<DirectX::XMFLOAT3> vCorners(8);
    ref.GetCorners(vCorners.data());
    DirectX::XMVECTOR planes[6];
    ref.GetPlanes( &planes[0], &planes[1], &planes[2], &planes[3], &planes[4], &planes[5] );
    std::vector<Math::Vector3> vv;
    std::copy( vCorners.begin(), vCorners.end(), std::back_inserter( vv ) );
    Math::BoundingFrustum bound( mat );
}

TEST( FrustumTest, FromViewProjMatrixLH )
{
    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH( Math::Vector3( 0, 10, -10 ), Math::Vector3( Math::kZero ), Math::Vector3( Math::kYUnitVector ) );
    DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveLH( 100, 50, 1, 100 );
    Math::Matrix4 mat( DirectX::XMMatrixMultiply( view, proj) );
    DirectX::BoundingFrustum ref( mat );
    std::vector<DirectX::XMFLOAT3> vCorners( 8 );
    ref.GetCorners( vCorners.data() );
    DirectX::XMVECTOR planes[6];
    ref.GetPlanes( &planes[0], &planes[1], &planes[2], &planes[3], &planes[4], &planes[5] );
    std::vector<Math::Vector3> vv;
    std::copy( vCorners.begin(), vCorners.end(), std::back_inserter( vv ) );
    Math::BoundingFrustum bound( mat );

    // Corners of the projection frustum in homogenous space.
    Math::Vector4 HomogenousPoints[8] =
    {
        {  1.0f,  -1.0f, 0.0f, 1.0f },   // right (at far plane)
        { -1.0f,  -1.0f, 0.0f, 1.0f },   // left
        {  1.0f,  1.0f, 0.0f, 1.0f },   // right (at far plane)
        { -1.0f,  1.0f, 0.0f, 1.0f },   // left

        {  1.0f,  -1.0f, 1.0f, 1.0f },   // right (at far plane)
        { -1.0f,  -1.0f, 1.0f, 1.0f },   // left
        {  1.0f,  1.0f, 1.0f, 1.0f },   // right (at far plane)
        { -1.0f,  1.0f, 1.0f, 1.0f },   // left
    };

    Math::Vector4 corners[8];
    for (int i = 0; i < 8; i++) {
        corners[i] = Math::Invert( Math::Matrix4( mat ) ) * HomogenousPoints[i];
        corners[i] /= corners[i].GetW();
    }
}

TEST( FrustumTest, FromViewProjMatrixLH2 )
{
    DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH( Math::Vector3( 0, 10, -10 ), Math::Vector3( Math::kZero ), Math::Vector3( Math::kYUnitVector ) );
    DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH( DirectX::XM_PIDIV4, 9.0f/16.0f, 1.0f, 20000.0f );
    Math::Matrix4 mat( DirectX::XMMatrixMultiply( view, proj ) );
    DirectX::BoundingFrustum ref( mat );
    std::vector<DirectX::XMFLOAT3> vCorners( 8 );
    ref.GetCorners( vCorners.data() );
    DirectX::XMVECTOR planes[6];
    ref.GetPlanes( &planes[0], &planes[1], &planes[2], &planes[3], &planes[4], &planes[5] );
    std::vector<Math::Vector3> vv;
    std::copy( vCorners.begin(), vCorners.end(), std::back_inserter( vv ) );
    Math::BoundingFrustum bound( mat );

    // Corners of the projection frustum in homogenous space.
    Math::Vector4 HomogenousPoints[8] =
    {
        {  1.0f,  -1.0f, 0.0f, 1.0f },   // right (at far plane)
        { -1.0f,  -1.0f, 0.0f, 1.0f },   // left
        {  1.0f,  1.0f, 0.0f, 1.0f },   // right (at far plane)
        { -1.0f,  1.0f, 0.0f, 1.0f },   // left

        {  1.0f,  -1.0f, 1.0f, 1.0f },   // right (at far plane)
        { -1.0f,  -1.0f, 1.0f, 1.0f },   // left
        {  1.0f,  1.0f, 1.0f, 1.0f },   // right (at far plane)
        { -1.0f,  1.0f, 1.0f, 1.0f },   // left
    };

    Math::Vector4 corners[8];
    for (int i = 0; i < 8; i++) {
        corners[i] = Math::Invert( Math::Matrix4( mat ) ) * HomogenousPoints[i];
        corners[i] /= corners[i].GetW();
    }
}

TEST(FrustumTest, IntersectBox)
{
    DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveLH( 100, 50, 1, 100 );
    Math::Matrix4 mat( proj );
    Math::BoundingFrustum bound( mat );
    Math::BoundingBox include( Math::Vector3(-1, -1, 1), Math::Vector3(1, 1, 3));
    EXPECT_TRUE(bound.IntersectBox( include ));
    Math::BoundingBox intersect( Math::Vector3(-1, -1, 0), Math::Vector3(1, 1, 3));
    EXPECT_TRUE(bound.IntersectBox( intersect ));
}
