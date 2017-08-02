#include "pch.h"
#include "OrthographicCamera.h"

using namespace Math;

void OrthographicCamera::SetOrthographic( float minX, float maxX, float minY, float maxY, float nearClip, float farClip )
{
    SetProjMatrix( OrthographicMatrix( minX, maxX, minY, maxY, nearClip, farClip, m_ReverseZ ) );
}
