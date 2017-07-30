#include "MikuCamera.h"
#include <cmath>

using namespace Math;

MikuCamera::MikuCamera() :
	m_bPerspective( true ),
	m_Distance( -45.f ),
	m_Position( 0.f, 10.f, 0.0 ),
    m_OrthFovY( 15.f )
{
	SetPerspective( 30.f*XM_PI/180.f, 9.0f / 16.0f, 0.5f, 20000.f );
	UpdateViewMatrix();
	UpdateProjMatrix();
}

void MikuCamera::SetOrthographic( float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip )
{
    m_OrthFovY = verticalFovRadians;
	m_AspectRatio = aspectHeightOverWidth;
	m_NearClip =  nearZClip;
	m_FarClip = farZClip;
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
	auto H = 2 * std::abs(m_Distance) * std::tan( m_OrthFovY*XM_PI / 180.f );
    auto W = H / m_AspectRatio;
    SetProjMatrix( OrthographicMatrix( W, H, m_NearClip, m_FarClip, m_ReverseZ ) );
}

void MikuCamera::UpdatePerspectiveMatrix()
{
    SetProjMatrix( PerspectiveMatrix( m_VerticalFOV, m_AspectRatio, m_NearClip, m_FarClip, m_ReverseZ) );
}
