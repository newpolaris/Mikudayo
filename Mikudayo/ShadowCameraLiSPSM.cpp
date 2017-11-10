#include "stdafx.h"

#include <functional> // EngineTuning.h
#include "ShadowCameraLiSPSM.h"
#include "Math/BoundingFrustum.h"
#include "EngineTuning.h"
#include "Scene.h"

using namespace Math;

const bool bLeftHand = false;
BoolVar m_bUseLispSM("Application/Camera/Use LispSM", false);

//calculates the up vector for the light coordinate frame
Vector3 calcUpVec( Vector3 viewDir, Vector3 lightDir )
{
	//left is the normalized vector perpendicular to lightDir and viewDir
	//this means left is the normalvector of the yz-plane from the paper
    Vector3 left = Cross( lightDir, viewDir );
	//we now can calculate the rotated(in the yz-plane) viewDir vector
	//and use it as up vector in further transformations
    return Normalize( Cross( left, lightDir ) );
}

VecPoint transformVecPoint( const VecPoint& B, Matrix4 mat )
{
    VecPoint R;
    R.reserve( B.size() );
    for (auto& b : B)
        R.push_back( mat.Transform( b ) );
    return R;
}

void ShadowCameraLiSPSM::UpdateMatrix( const Scene& scene, Math::Vector3 LightDirection, const BaseCamera& Camera )
{
    Matrix4 view = Camera.GetViewMatrix();
    Matrix4 proj = Camera.GetProjMatrix();
    Vector3 eyePos = Camera.GetPosition();
    Vector3 viewDir = Camera.GetForwardVec();
    // From light
    Vector3 lightDir = Normalize(LightDirection);

    //  these are the limits specified by the physical camera
    //  gamma is the "tilt angle" between the light and the view direction.
    BoundingFrustum sceneFrustum( proj*view );
    BoundingBox sceneAABox;
    for (auto& node : scene) {
        auto box = node->GetBoundingBox();
        sceneAABox.Merge( box );
    }
    VecPoint points;
    calcFocusedLightVolumePoints( points, lightDir, sceneFrustum, sceneAABox );
    std::vector<Vector3> B = points;
    if (!m_bUseLispSM)
        CalcUniformShadowMtx( eyePos, lightDir, viewDir, B );
    else
        CalcLispSMMtx( eyePos, lightDir, viewDir, B );
}

void ShadowCameraLiSPSM::CalcLispSMMtx( const Vector3& eyePos, const Vector3& lightDir, const Vector3& viewDir, const VecPoint& B )
{
    float dotProd = Dot( viewDir, lightDir );
    float sinGamma = Sqrt( 1.0f - dotProd*dotProd );

    std::vector<Vector3> Bcopy = B;
    Vector3 up = calcUpVec( viewDir, lightDir );

    // zaxis in light space
    const Vector3 zaxis = bLeftHand ? lightDir: -lightDir;

	//temporal light View
	//look from position(eyePos)
	//into direction(lightDir)
	//with up vector(up)
    Matrix4 lightView = MatrixLookDirection( eyePos, zaxis, up );

	//transform the light volume points from world into light space
    VecPoint B2 = transformVecPoint( B, lightView );

    BoundingBox box( B2 );
    Matrix4 lightProjection;
    {
        float nearDist = 1.0;
        //use the formulas of the paper to get n (and f)
        float factor = 1.0f / sinGamma;
        float z_n = factor * nearDist;
        float d = Abs( box.GetMax().GetY() - box.GetMin().GetY() );
        float z_f = z_n + d * sinGamma;
        float n = (z_n + Sqrt( z_f * z_n )) / sinGamma;
        float f = n + d;
        Vector3 pos = eyePos + up * -(n - nearDist);
        lightView = MatrixLookDirection( pos, zaxis, up );

        float a, b;
        a = f / (f - n);
        b = -a * n;

		//one possibility for a simple perspective transformation matrix
		//with the two parameters n(near) and f(far) in y direction
        Matrix4 lispMtx(
            Vector4( 1, 0, 0, 0 ),
            Vector4( 0, a, 0, 1 ),
            Vector4( 0, 0, 1, 0 ),
            Vector4( 0, b, 0, 0 ) );

        lightProjection = lispMtx * lightView;

        Bcopy = transformVecPoint( Bcopy, lightProjection );

		//calculate the cubic hull (an AABB)
		//of the light space extents of the intersection body B
		//and save the two extreme points min and max
        Math::BoundingBox aabb( Bcopy );
        lightProjection = MatrixScaleTranslateToFit( aabb, m_ReverseZ );
        lightProjection = lightProjection * lispMtx;
    }

    UpdateViewProjMatrix( lightView, lightProjection );
}


void ShadowCameraLiSPSM::CalcUniformShadowMtx( const Vector3& eyePos, const Vector3& lightDir, const Vector3& viewDir, const VecPoint& B )
{
    // zaxis in light space
    const Vector3 zaxis = bLeftHand ? lightDir: -lightDir;
    Matrix4 lightView = MatrixLookDirection( eyePos, zaxis, viewDir );

	//transform the light volume points from world into light space
    VecPoint B2 = transformVecPoint( B, lightView );

	//calculate the cubic hull (an AABB)
	//of the light space extents of the intersection body B
	//and save the two extreme points min and max
    BoundingBox aabb(B2);

	//refit to unit cube
	//this operation calculates a scale translate matrix that
	//maps the two extreme points min and max into (-1,-1,-1) and (1,1,1)
    Matrix4 lightProj = MatrixScaleTranslateToFit( aabb, m_ReverseZ );

    UpdateViewProjMatrix( lightView, lightProj );
}