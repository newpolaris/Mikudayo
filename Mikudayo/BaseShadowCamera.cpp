#include "stdafx.h"
#include "BaseShadowCamera.h"

using namespace Math;

void BaseShadowCamera::UpdateViewProjMatrix( const Matrix4& View, const Matrix4& Projection )
{
    m_ViewMatrix = View;
    m_ProjMatrix = Projection;
    m_ViewProjMatrix = Projection * View;
    m_ClipToWorld = Invert(m_ViewProjMatrix);
	m_FrustumVS = Frustum( m_ProjMatrix );
	m_FrustumWS = m_CameraToWorld * m_FrustumVS;

    // Transform from clip space to texture space
    m_ShadowMatrix = Matrix4( AffineTransform( Matrix3::MakeScale( 0.5f, -0.5f, 1.0f ), Vector3( 0.5f, 0.5f, 0.0f ) ) ) * m_ViewProjMatrix;
}
