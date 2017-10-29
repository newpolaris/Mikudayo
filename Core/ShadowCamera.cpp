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

using namespace Math;
using namespace GameCore;

BoolVar RecenterCamera( "Graphics/Camera/Recenter", true );

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

void ShadowCamera::UpdateMatrix( Vector3 LightDirection, Vector3 ShadowCenter, Vector3 ShadowBounds, BaseCamera& camera )
{
    SetPosition( ShadowCenter );

    SetLookDirection( LightDirection, Vector3(kYUnitVector) );
    SetProjMatrix( OrthographicMatrix(ShadowBounds.GetX(), ShadowBounds.GetY(), -ShadowBounds.GetZ(), ShadowBounds.GetZ(), m_ReverseZ) );
    SetProjMatrix( PerspectiveMatrix(XM_PI / 4, 1.0f, 10.f, 1000, m_ReverseZ ) );

	m_ViewMatrix = Matrix4(~m_CameraToWorld);
	m_ViewProjMatrix = m_ProjMatrix * m_ViewMatrix;
    // Update();

    float c_near = 1, c_far = 1000;
    auto m_matViewPrime = camera.GetViewMatrix();
    auto m_matProjPrime = camera.GetProjMatrix();
    auto m_matViewProjPrime = m_matProjPrime * m_matViewPrime;

	// Generate the shadow buffer transform matrix and the
	// corresponding texture transform matrix (the shadow buffer
	// matrix maps from screen space to shadow-buffer pre-projection
	// space; the texture transform matrix just tacks on the extra
	// texture scaling.)

    // For a directional light, basically we want to look at the
    // screen-space cube from some point on the "infinite plane"
    // (past the end of zFar).

    Vector3 m_lightDir = Normalize(-LightDirection);
    Matrix4 view, proj;
    // Light position in post-perspective screen space.
    Vector3 lightdir = camera.GetViewMatrix().Get3x3() * m_lightDir;
    Vector4 lightPos = camera.GetProjMatrix() * Vector4( lightdir, 0 );
    lightPos = camera.GetViewProjMatrix() * Vector4( m_lightDir, 0 );
    lightPos /= lightPos.GetW();

    Vector3 center( 0, 0, 0.5 );
    Vector3 yaxis( 0, 1, 0 );
    if (fabsf( Dot( Vector3( lightPos ), yaxis ) ) > 0.99f)
        yaxis = Vector3( 0, 0, 1 );;

    float fRadius = 1.0f;
    Vector3 lightpos = Vector3(lightPos);
    float fDist = Length( lightpos - center );
    view = Matrix4(XMMatrixLookAtRH( lightpos, center, yaxis ));
    float	fAngle = 2.0f * asinf( fRadius / fDist );
    float	n = fDist - fRadius * 2.f;
    float	f = fDist + fRadius * 2.f;
    if (n < 0.001f) { n = 0.001f; }

    proj = PerspectiveMatrix( fAngle, 1.0, n, f, m_ReverseZ );
    m_ProjMatrix = proj * view * Invert(m_matViewPrime) * m_matProjPrime * m_matViewPrime;
    m_ViewMatrix = Matrix4(kIdentity);
    m_ViewProjMatrix = m_ProjMatrix * m_ViewMatrix;
    //  build the composite matrix that transforms from world space into post-projective light space
    m_ShadowMatrix = m_ViewProjMatrix;
}

