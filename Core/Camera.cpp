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
#include "Camera.h"
#include <cmath>

using namespace Math;

const bool BaseCamera::m_ReverseZ = false;

//
// 'forward' is inverse look direction (right hand coord)
// So in cross calcuration to calc right and up vector
// Element order is chaged to correct equation
//
void BaseCamera::SetLookDirection( Vector3 forward, Vector3 up )
{
	// Given, but ensure normalization
	Scalar forwardLenSq = LengthSquare(forward);
	forward = Select(forward * RecipSqrt(forwardLenSq), -Vector3(kZUnitVector), forwardLenSq < Scalar(0.000001f));

	// Deduce a valid, orthogonal right vector

	Vector3 right = Cross(forward, up); // forward = -look
	Scalar rightLenSq = LengthSquare(right);
	right = Select(right * RecipSqrt(rightLenSq), Quaternion(Vector3(kYUnitVector), -XM_PIDIV2) * forward, rightLenSq < Scalar(0.000001f));

	// Compute actual up vector
	up = Cross(right, forward); // forward = -look

	// Finish constructing basis
	m_Basis = Matrix3(right, up, -forward); // -forward = look
	m_CameraToWorld.SetRotation(Quaternion(m_Basis));
}

void BaseCamera::Update()
{
	m_PreviousViewProjMatrix = m_ViewProjMatrix;

	m_ViewMatrix = Matrix4(~m_CameraToWorld);
	m_ViewProjMatrix = m_ProjMatrix * m_ViewMatrix;
    m_ClipToWorld = Invert(m_ViewProjMatrix);
	m_ReprojectMatrix = m_PreviousViewProjMatrix * m_ClipToWorld;

	m_FrustumVS = Frustum( m_ProjMatrix );
	m_FrustumWS = m_CameraToWorld * m_FrustumVS;
}

Matrix4 BaseCamera::PerspectiveMatrix( float VerticalFOV, float AspectRatio, float NearClip, float FarClip, bool bReverseZ )
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
		Vector4( 0.0f, Y, 0.0f, 0.0f ),
		Vector4( 0.0f, 0.0f, Q1, -1.0f ),
		Vector4( 0.0f, 0.0f, Q2, 0.0f )
		);
}

Matrix4 BaseCamera::OrthogonalMatrix( float W, float H, float NearClip, float FarClip, bool bReverseZ )
{
	auto X = 2 / W;
	auto Y = 2 / H;

	float Q1, Q2;
	if (bReverseZ)
	{
		Q1 = 1 / (FarClip - NearClip);
		Q2 = Q1 * FarClip;
	}
	else
	{
		Q1 = 1 / (NearClip - FarClip);
		Q2 = Q1 * NearClip;
	}

	return Matrix4(
		Vector4( X, 0.f, 0.f, 0.f ),
		Vector4( 0.f, Y, 0.f, 0.f ),
		Vector4( 0.f, 0.f, Q1, 0.f ),
		Vector4( 0.f, 0.f, Q2, 1.f ) 
	);
}

void Camera::UpdateProjMatrix( void )
{
    SetProjMatrix(PerspectiveMatrix( m_VerticalFOV, m_AspectRatio, m_NearClip, m_FarClip, m_ReverseZ ));
}
