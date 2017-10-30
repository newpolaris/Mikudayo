//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "pch.h"
#include "ShadowCamera.h"
#include "Math/BoundingBox.h"

using namespace Math;
using namespace GameCore;

BoolVar RecenterCamera( "Graphics/Camera/Recenter", true );

namespace {
	/** transform from normal to light space */
	const Matrix4 msNormalToLightSpace(
		Vector4(1,  0,  0,  0),		// x
		Vector4(0,  0, -1,  0),		// y
		Vector4(0,  1,  0,  0),		// z
		Vector4(0,  0,  0,  1));	// w
	/** transform  from light to normal space */
	const Matrix4 msLightSpaceToNormal(
		Vector4(1,  0,  0,  0),		// x
		Vector4(0,  0,  1,  0),		// y
		Vector4(0, -1,  0,  0),		// z
		Vector4(0,  0,  0,  1));	// w

}

Matrix4 PerspectiveMatrixY( float VerticalFOV, float AspectRatio, float NearClip, float FarClip, bool bReverseZ )
{
    float Y = 1.0f / std::tanf( VerticalFOV * 0.5f );
    float X = Y * AspectRatio;

    float Q1, Q2;

    //
    // ReverseZ puts far plane at Z=0 and near plane at Z=1.  This is never a bad idea, and it's
    // actually a great idea with F32 depth buffers to redistribute precision more evenly across
    // the entire range.  It requires clearing Z to 0.0f and using a GREATER variant depth test.
    // Some care must also be done to properly reconstruct linear W in a pixel shader from hyperbolic Z.
    //
    if (bReverseZ)
    {
        Q1 = NearClip / (FarClip - NearClip);
        Q2 = Q1 * FarClip;
    }
    else
    {
        Q1 = FarClip / (NearClip - FarClip);
        Q2 = Q1 * NearClip;
    }

    return Matrix4(
        Vector4( X, 0.0f, 0.0f, 0.0f ),
        Vector4( 0.0f, Q1, 0.0f, -1.0f ),
        Vector4( 0.0f, 0.0f, Y, 0.0f ),
        Vector4( 0.0f, Q2, 0, 0.0f )
    );
}

void ShadowCamera::UpdateMatrix(
    Vector3 LightDirection, Vector3 ShadowCenter, Vector3 ShadowBounds,
    uint32_t BufferWidth, uint32_t BufferHeight, uint32_t BufferPrecision )
{
    // Converts world units to texel units so we can quantize the camera position to whole texel units
    Vector3 RcpDimensions = Recip(ShadowBounds);
    Vector3 QuantizeScale = Vector3((float)BufferWidth, (float)BufferHeight, (float)((1 << BufferPrecision) - 1)) * RcpDimensions;

    //
    // Recenter the camera at the quantized position
    //
    if (RecenterCamera)
    {
        // Transform to view space
        ShadowCenter = ~GetRotation() * ShadowCenter;
        // Scale to texel units, truncate fractional part, and scale back to world units
        ShadowCenter = Floor( ShadowCenter * QuantizeScale ) / QuantizeScale;
        // Transform back into world space
        ShadowCenter = GetRotation() * ShadowCenter;
    }
    SetPosition( ShadowCenter );

    SetLookDirection( LightDirection, Vector3(kYUnitVector) );
    SetProjMatrix( OrthographicMatrix(ShadowBounds.GetX(), ShadowBounds.GetY(), -ShadowBounds.GetZ(), ShadowBounds.GetZ(), m_ReverseZ) );
    SetProjMatrix( PerspectiveMatrixY(XM_PI / 4, 1.0f, 0.f, 1, m_ReverseZ ) );

    Update();

    // Transform from clip space to texture space
    m_ShadowMatrix = Matrix4( AffineTransform( Matrix3::MakeScale( 0.5f, -0.5f, 1.0f ), Vector3(0.5f, 0.5f, 0.0f) ) ) * m_ViewProjMatrix;
    m_ShadowMatrix = m_ViewProjMatrix;
}

void ShadowCamera::UpdateMatrix( Vector3 LightVector, Vector3 ShadowCenter, Vector3 ShadowBounds, BaseCamera& camera )
{
    SetPosition( ShadowCenter );

    SetLookDirection( LightVector, Vector3(kYUnitVector) );
    SetProjMatrix( OrthographicMatrix(ShadowBounds.GetX(), ShadowBounds.GetY(), -ShadowBounds.GetZ(), ShadowBounds.GetZ(), m_ReverseZ) );
    SetProjMatrix( PerspectiveMatrix(XM_PI / 4, 1.0f, 10.f, 1000, m_ReverseZ ) );

	m_ViewMatrix = Matrix4(~m_CameraToWorld);
	m_ViewProjMatrix = m_ProjMatrix * m_ViewMatrix;

    Vector3 lightDir = Normalize(-LightVector);

    //  these are the limits specified by the physical camera
    //  gamma is the "tilt angle" between the light and the view direction.
    float fCosGamma = Dot( lightDir, camera.GetForwardVec());
    float fLSPSM_NoptWeight = 1.0f;
    float ZNEAR_MIN = 1.0f, ZFAR_MAX = 800.f;
    float m_zNear = ZNEAR_MIN, m_zFar = ZFAR_MAX;

    Frustum eyeFrustum = camera.GetViewSpaceFrustum();
    std::vector<Vector3> bodyB;
    FrustumCorner corner = eyeFrustum.GetFrustumCorners();
    std::copy( corner.begin(), corner.end(), std::back_inserter( bodyB ) );

    //  compute the "light-space" basis, using the algorithm described in the paper
    //  note:  since bodyB is defined in eye space, all of these vectors should also be defined in eye space
    Vector3 rightVector, upVector, viewVector;

    Matrix4 cameraView = camera.GetViewMatrix();
    const Vector3 eyeVector( 0.f, 0.f, -1.f );  // eye vector in eye space is always -Z 

    //  note: lightDir points away from the scene, so it is already the "negative" up direction;
    //  no need to re-negate it.
    upVector = cameraView.Get3x3() * lightDir;
    rightVector = Cross( eyeVector, upVector );
    rightVector = Normalize( rightVector );
    viewVector = Cross( upVector, rightVector );

    Matrix4 lightSpaceBasis( rightVector, upVector, viewVector, Vector3());
    lightSpaceBasis = Transpose( lightSpaceBasis );

    //  rotate all points into this new basis
    for (auto& body : bodyB)
        body = lightSpaceBasis.Transform(body);

    Math::BoundingBox lightSpaceBox( bodyB );
    Vector3 lightSpaceOrigin;
    //  for some reason, the paper recommended using the x coordinate of the xformed viewpoint as
    //  the x-origin for lightspace, but that doesn't seem to make sense...  instead, we'll take
    //  the x-midpt of body B (like for the Y axis)
    lightSpaceOrigin = lightSpaceBox.GetCenter();
    float sinGamma = sqrtf( 1.f - fCosGamma*fCosGamma );
    //  use average of the "real" near/far distance and the optimized near/far distance to get a more pleasant result
    float Nopt0 = m_zNear + sqrtf( m_zNear*m_zFar );
    float Nopt1 = ZNEAR_MIN + sqrtf( ZNEAR_MIN*ZFAR_MAX );
    float fLSPSM_Nopt = (Nopt0 + Nopt1) / (2.f*sinGamma);
    //  add a constant bias, to guarantee some minimum distance between the projection point and the near plane
    fLSPSM_Nopt += 0.1f;
    //  now use the weighting to scale between 0.1 and the computed Nopt
    float Nopt = 0.1f + fLSPSM_NoptWeight * (fLSPSM_Nopt - 0.1f);

    lightSpaceOrigin.SetZ( lightSpaceBox.GetMin().GetZ() - Nopt );

    //  xlate all points in lsBodyB, to match the new lightspace origin, and compute the fov and aspect ratio
    float maxx = 0.f, maxy = 0.f, maxz = 0.f;

    std::vector<Vector3>::iterator ptIt = bodyB.begin();

    while (ptIt != bodyB.end())
    {
        Vector3 tmp = *ptIt++ - lightSpaceOrigin;
        ASSERT( tmp.GetZ() > 0.f );
        maxx = std::max( maxx, fabsf( tmp.GetX() / tmp.GetZ() ) );
        maxy = std::max( maxy, fabsf( tmp.GetY() / tmp.GetZ() ) );
        maxz = std::max( maxz, float(tmp.GetZ()) );
    }

    float fovy = atanf( maxy );
    float fovx = atanf( maxx );

    Matrix4 lsTranslate, lsPerspective;
    lsTranslate = Matrix4::MakeTranslate( -lightSpaceOrigin );
    lsPerspective = PerspectiveMatrix( 2.f*maxx*Nopt, 2.f*maxy*Nopt, Nopt, maxz, m_ReverseZ );

    lightSpaceBasis = lsPerspective * lsTranslate * lightSpaceBasis;

    //  now rotate the entire post-projective cube, so that the shadow map is looking down the Y-axis
    Matrix4 lsPermute(
        Vector4( 1.f, 0.f, 0.f, 0.f ),
        Vector4( 0.f, 0.f, -1.f, 0.f ),
        Vector4( 0.f, 1.f, 0.f, 0.f ),
        Vector4( 0.f, -0.5f, 1.5f, 1.f ) );

    Matrix4 lsOrtho = OrthographicMatrix( 2.f, 1.f, 0.5f, 2.5f, m_ReverseZ );

    m_ShadowMatrix = lsOrtho * lsPermute * lightSpaceBasis * camera.GetViewMatrix();

    m_ViewMatrix = Matrix4(kIdentity);
    m_ProjMatrix = m_ShadowMatrix;
    m_ViewProjMatrix = m_ShadowMatrix;
    //  build the composite matrix that transforms from world space into post-projective light space
    m_ShadowMatrix = m_ViewProjMatrix;
}

