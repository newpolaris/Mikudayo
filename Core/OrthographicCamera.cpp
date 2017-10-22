#include "pch.h"
#include "OrthographicCamera.h"

using namespace Math;

OrthographicCamera::OrthographicCamera()
{
    SetOrthographic( -1.f, 1.f, -1.f, 1.f, 0.f, 100.f );
}

void OrthographicCamera::SetOrthographic( float minX, float maxX, float minY, float maxY, float nearClip, float farClip )
{
    m_NearClip = nearClip;
    m_FarClip = farClip;
    SetProjMatrix( OrthographicMatrix( minX, maxX, minY, maxY, nearClip, farClip, m_ReverseZ ) );
}
