#pragma once

#include "Vector.h"

namespace Math
{
    bool NearRelative( const Vector3& v1, const Vector3& v2, const Vector3& eps );
    bool NearRelative( const Vector4& v1, const Vector4& v2, const Vector4& eps );
    bool NearRelative( const Matrix3& m1, const Matrix3& m2, const Vector3& eps );
    bool NearRelative( const Matrix4& m1, const Matrix4& m2, const Vector4& eps );

    Matrix4 PerspectiveMatrix( float VerticalFOV, float AspectRatio, float NearClip, float FarClip, bool bReverseZ );
    Matrix4 OrthographicMatrix( float Width, float Height, float NearClip, float FarClip, bool bReverseZ );
    Matrix4 OrthographicMatrix( float Left, float Right, float Bottom, float Top, float NearClip, float FarClip, bool bReverseZ );
}
