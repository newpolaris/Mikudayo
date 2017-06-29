#include "MikuCamera.h"
#include <cmath>

using namespace Math;

MikuCamera::MikuCamera() :
	m_ReverseZ( false ),
	m_bPerspective( true ),
	m_Distance( -45.f ),
	m_Position( 0.f, 10.f, 0.0 )
{
	SetPerspective( 30.f*XM_PI/180.f, 9.0f / 16.0f, 0.5f, 20000.f );
	UpdateViewMatrix();
	UpdateProjMatrix();
}

void MikuCamera::SetPerspective( float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip )
{
	m_VerticalFOV = verticalFovRadians;
	m_AspectRatio = aspectHeightOverWidth;
	m_NearClip =  nearZClip;
	m_FarClip = farZClip;
}

//
// MMD Editor used LH coordinate, but UI shows RH coordinate
// the file VMD, PMD uses LH coordicnate
//
Vector3 MikuCamera::GetDistanceVector() const
{
	return Vector3( 0.f, 0.f, -m_Distance );
}

void MikuCamera::UpdateViewMatrix()
{
	OrthogonalTransform trans( m_Rotation, m_Position );
	auto dist = GetDistanceVector();
	auto cameraPos = trans * dist;
	auto up = m_Rotation * Vector3( kYUnitVector );
	SetEyeAtUp( cameraPos, m_Position, up );
}

void MikuCamera::UpdateProjMatrix()
{
	if (m_bPerspective)
		UpdatePerspectiveMatrix();
	else
		UpdateOrthogonalMatrix();
}

void MikuCamera::UpdateOrthogonalMatrix()
{
	//
	// (Default fovY) / 2 = 30.f / 2 = 15.f
	//
	auto W = 2 * std::abs(m_Distance) * std::tan( 15.f*XM_PI / 180.f );
	auto Y = 2 / W;
	auto X = Y * m_AspectRatio;

	float Q1, Q2;
	if (m_ReverseZ)
	{
		Q1 = 1 / (m_FarClip - m_NearClip);
		Q2 = Q1 * m_FarClip;
	}
	else
	{
		Q1 = 1 / (m_NearClip - m_FarClip);
		Q2 = Q1 * m_FarClip;
	}

	SetProjMatrix( Matrix4(
		Vector4( X, 0.f, 0.f, 0.f ),
		Vector4( 0.f, Y, 0.f, 0.f ),
		Vector4( 0.f, 0.f, Q1, 0.f ),
		Vector4( 0.f, 0.f, Q2, 1.f ) 
	) );
}

void MikuCamera::UpdatePerspectiveMatrix()
{
	float Y = 1.0f / std::tanf( m_VerticalFOV * 0.5f );
	float X = Y * m_AspectRatio;

	float Q1, Q2;

	//
	// ReverseZ puts far plane at Z=0 and near plane at Z=1.  This is never a bad idea, and it's
	// actually a great idea with F32 depth buffers to redistribute precision more evenly across
	// the entire range.  It requires clearing Z to 0.0f and using a GREATER variant depth test.
	// Some care must also be done to properly reconstruct linear W in a pixel shader from hyperbolic Z.
	//
	if (m_ReverseZ)
	{
		Q1 = m_NearClip / (m_FarClip - m_NearClip);
		Q2 = Q1 * m_FarClip;
	}
	else
	{
		Q1 = m_FarClip / (m_NearClip - m_FarClip);
		Q2 = Q1 * m_NearClip;
	}

	SetProjMatrix( Matrix4(
		Vector4( X, 0.0f, 0.0f, 0.0f ),
		Vector4( 0.0f, Y, 0.0f, 0.0f ),
		Vector4( 0.0f, 0.0f, Q1, -1.0f ),
		Vector4( 0.0f, 0.0f, Q2, 0.0f )
		) );
}
