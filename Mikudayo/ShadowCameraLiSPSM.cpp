#include "stdafx.h"

#include <functional> // EngineTuning.h
#include "ShadowCameraLiSPSM.h"
#include "Math/BoundingFrustum.h"
#include "EngineTuning.h"
#include "Scene.h"

using namespace Math;

BoolVar m_bUseLispSM("Application/Camera/Use LispSM", true);

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

Matrix4 look( const Vector3 pos, const Vector3 dir, const Vector3 up )
{
    Vector3 dirN;
    Vector3 upN;
    Vector3 lftN;

    lftN = Cross( dir, up );
    lftN = Normalize( lftN );

    upN = Cross( lftN, dir );
    upN = Normalize( upN );
    dirN = Normalize( dir );

    Matrix4 lightSpaceBasis( -lftN, upN, dirN, Vector3( kZero ) );
    lightSpaceBasis = Transpose( lightSpaceBasis );
    Matrix4 lightView = lightSpaceBasis * Matrix4::MakeTranslate( -pos );
    return lightView;
}

VecPoint transformVecPoint( const VecPoint& B, Matrix4 mat )
{
    VecPoint R;
    R.reserve( B.size() );
    for (auto& b : B)
        R.push_back( mat.Transform( b ) );
    return R;
}

Matrix4 scaleTranslateToFit( BoundingBox& aabb, bool bReverseZ )
{
    return OrthographicMatrix(
        aabb.m_Min.GetX(), aabb.m_Max.GetX(),
        aabb.m_Min.GetY(), aabb.m_Max.GetY(),
        aabb.m_Min.GetZ(), aabb.m_Max.GetZ(),
        bReverseZ );
}

void ShadowCameraLiSPSM::CalcLispSMMtx( Vector3 eyePos, Vector3 lightDir, Vector3 viewDir, VecPoint& B )
{
    float dotProd = Dot( viewDir, lightDir );
    float sinGamma = Sqrt( 1.0f - dotProd*dotProd );

    std::vector<Vector3> Bcopy = B;
    Vector3 up = calcUpVec( viewDir, lightDir );

	//temporal light View
	//look from position(eyePos)
	//into direction(lightDir)
	//with up vector(up)
    Matrix4 lightView = look( eyePos, lightDir, up );

	//transform the light volume points from world into light space
    B = transformVecPoint( B, lightView );

    BoundingBox box( B );
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
        lightView = look( pos, lightDir, up );

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
        lightProjection = scaleTranslateToFit( aabb, m_ReverseZ );
        lightProjection = lightProjection * lispMtx;
    }

    UpdateViewProjMatrix( lightView, lightProjection );
}

void ShadowCameraLiSPSM::UpdateMatrix( const Scene& Model, Math::Vector3 LightDirection, const BaseCamera& Camera )
{
#if 0
    Matrix4 view = Camera.GetViewMatrix();
    Matrix4 proj = Camera.GetProjMatrix();
    Vector3 eyePos = Camera.GetPosition();
    Vector3 viewDir = -Camera.GetForwardVec();

    Vector3 lightDir = Normalize(LightDirection);

    //  these are the limits specified by the physical camera
    //  gamma is the "tilt angle" between the light and the view direction.
    BoundingFrustum sceneFrustum( proj*view );
    Vector3 minVec( std::numeric_limits<float>::max() ), maxVec( std::numeric_limits<float>::lowest() );
    for (auto& model : Model) {
        auto box = model->GetBoundingBox();
        minVec = Min( box.GetMin(), minVec );
        maxVec = Max( box.GetMax(), maxVec );
    }
    BoundingBox sceneAABox( minVec, maxVec );
    VecPoint points;
    calcFocusedLightVolumePoints( points, lightDir,
        sceneFrustum, sceneAABox );
    std::vector<Vector3> B = points;
    if (!m_bUseLispSM)
        CalcUniformShadowMtx( eyePos, lightDir, viewDir, B );
    else
        CalcLispSMMtx( eyePos, lightDir, viewDir, B );
#endif
}

void ShadowCameraLiSPSM::CalcUniformShadowMtx( Vector3 eyePos, Vector3 lightDir, Vector3 viewDir, VecPoint& B )
{
    Matrix4 lightView = look( eyePos, lightDir, viewDir );

	//transform the light volume points from world into light space
    B = transformVecPoint( B, lightView );

	//calculate the cubic hull (an AABB)
	//of the light space extents of the intersection body B
	//and save the two extreme points min and max
    BoundingBox aabb(B);

	//refit to unit cube
	//this operation calculates a scale translate matrix that
	//maps the two extreme points min and max into (-1,-1,-1) and (1,1,1)
    Matrix4 lightProj = scaleTranslateToFit( aabb, m_ReverseZ );

    UpdateViewProjMatrix( lightView, lightProj );
}