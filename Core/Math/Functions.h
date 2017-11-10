#pragma once

#include "Vector.h"

namespace Math
{
    class BoundingBox;

    bool NearRelative( const Vector3& v1, const Vector3& v2, const Vector3& eps );
    bool NearRelative( const Vector4& v1, const Vector4& v2, const Vector4& eps );
    bool NearRelative( const Matrix3& m1, const Matrix3& m2, const Vector3& eps );
    bool NearRelative( const Matrix4& m1, const Matrix4& m2, const Vector4& eps );

    Matrix4 MatrixLookAt( const Vector3& EyePosition, const Vector3& FocusPosition, const Vector3& UpDirection );
    Matrix4 MatrixLookAtLH( const Vector3& EyePosition, const Vector3& FocusPosition, const Vector3& UpDirection );
    Matrix4 MatrixLookAtRH( const Vector3& EyePosition, const Vector3& FocusPosition, const Vector3& UpDirection );
    Matrix4 MatrixLookDirection( const Vector3& EyePosition, const Vector3& Direction, const Vector3 UpDirection );
    Matrix4 PerspectiveMatrix( float VerticalFOV, float AspectRatio, float NearClip, float FarClip, bool bReverseZ );
    Matrix4 OrthographicMatrix( float Width, float Height, float NearClip, float FarClip, bool bReverseZ );
    Matrix4 OrthographicMatrix( float Left, float Right, float Bottom, float Top, float NearClip, float FarClip, bool bReverseZ );
    Matrix4 OrthographicMatrixLH( float Left, float Right, float Bottom, float Top, float NearClip, float FarClip, bool bReverseZ );
    Matrix4 OrthographicMatrixRH( float Left, float Right, float Bottom, float Top, float NearClip, float FarClip, bool bReverseZ );
    Matrix4 MatrixScaleTranslateToFitLH( const BoundingBox& aabb, bool bReverseZ );
    Matrix4 MatrixScaleTranslateToFitRH( const BoundingBox& aabb, bool bReverseZ );
    Matrix4 MatrixScaleTranslateToFit( const BoundingBox& aabb, bool bReverseZ );
}
