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
    Matrix4 invViewProj = Invert( camera.GetViewProjMatrix() );
    Vector3 cameraPos = camera.GetPosition();

    //  these are the limits specified by the physical camera
    //  gamma is the "tilt angle" between the light and the view direction.
    float fCosGamma = Dot( lightDir, -camera.GetForwardVec());

    Frustum corners = camera.GetWorldSpaceFrustum();
    FrustumCorner corner = corners.GetFrustumCorners();
    std::vector<Vector3> bodyB;
    std::copy( corner.begin(), corner.end(), std::back_inserter( bodyB ) );

    Vector3 rightVector, upVector, lookVector;

    Matrix4 cameraView = camera.GetViewMatrix();
    const Vector3 backVector = -camera.GetForwardVec();

    upVector = lightDir;
    rightVector = Cross( upVector, backVector );
    lookVector = Cross( rightVector, upVector );

    Matrix4 lightSpaceBasis( rightVector, upVector, lookVector, Vector3(kZero));
    lightSpaceBasis = Transpose( lightSpaceBasis );
    Matrix4 lightView = lightSpaceBasis * Matrix4::MakeTranslate( -cameraPos );

    for (auto& body : bodyB)
        body = lightView.Transform(body);

    Math::BoundingBox aabb( bodyB );
    Vector3 nearPt( kZero );
    nearPt = invViewProj.Transform( nearPt );

    Scalar nearDist = Length( nearPt - cameraPos );
    float sinGamma = sqrtf( 1.f - fCosGamma*fCosGamma );
    float factor = 1.0f / sinGamma;
    float z_n = factor * nearDist;
    float d = Abs( aabb.GetMax().GetY() - aabb.GetMin().GetY() );
    float z_f = z_n + d * sinGamma;
    float n = (z_n + Sqrt( z_f * z_n )) / sinGamma;
    float f = n + d;
    Vector3 pos = cameraPos + upVector * -(n - nearDist);
    Matrix4 viewMatrix = lightSpaceBasis * Matrix4::MakeTranslate( -pos );

    float a, b;
    if (m_ReverseZ)
    {
        a = -n / (f - n);
        b = a * f;
    }
    else
    {
        a = -f / (n - f);
        b = a * n;
    }

    Matrix4 lispMatrix(
        Vector4( 1, 0, 0, 0 ),
        Vector4( 0, a, 0, 1 ),
        Vector4( 0, 0, 1, 0 ),
        Vector4( 0, b, 0, 0 ) );

    Matrix4 lightProjection = lispMatrix * viewMatrix;

    m_ShadowMatrix = lispMatrix * viewMatrix;

    m_ViewMatrix = viewMatrix;
    m_ProjMatrix = lispMatrix;
    m_ViewProjMatrix = m_ShadowMatrix;
    //  build the composite matrix that transforms from world space into post-projective light space
    m_ShadowMatrix = m_ViewProjMatrix;

    m_ClipToWorld = Invert(m_ViewProjMatrix);
	m_FrustumVS = Frustum( m_ProjMatrix );
    m_FrustumWS = Invert( m_ViewMatrix ) * m_FrustumVS;
}

