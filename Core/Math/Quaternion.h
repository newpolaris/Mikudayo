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

#pragma once

#include "Vector.h"

namespace Math
{
	class Quaternion
	{
	public:
		INLINE Quaternion() { m_vec = XMQuaternionIdentity(); }
		INLINE Quaternion( const Vector3& axis, const Scalar& angle ) { m_vec = XMQuaternionRotationAxis( axis, angle ); }
		INLINE Quaternion( float pitch, float yaw, float roll ) { m_vec = XMQuaternionRotationRollPitchYaw( pitch, yaw, roll ); }
        INLINE Quaternion( float x, float y, float z, float w ) { m_vec = XMVectorSet( x, y, z, w ); }
		INLINE Quaternion( const Scalar& w, const Vector3& vec  ) { m_vec = XMVectorSet( vec.GetX(), vec.GetY(), vec.GetZ(), w ); }
		INLINE explicit Quaternion( const XMFLOAT4& v ) { m_vec = XMLoadFloat4(&v); }
		INLINE explicit Quaternion( const XMMATRIX& matrix ) { m_vec = XMQuaternionRotationMatrix( matrix ); }	
		INLINE explicit Quaternion( FXMVECTOR vec ) { m_vec = vec; }
		INLINE explicit Quaternion( EIdentityTag ) { m_vec = XMQuaternionIdentity(); }

		INLINE operator XMVECTOR() const { return m_vec; }

		INLINE Scalar GetX() const { return Scalar(XMVectorSplatX(m_vec)); }
		INLINE Scalar GetY() const { return Scalar(XMVectorSplatY(m_vec)); }
		INLINE Scalar GetZ() const { return Scalar(XMVectorSplatZ(m_vec)); }
		INLINE Scalar GetW() const { return Scalar(XMVectorSplatW(m_vec)); }
		INLINE void SetX( Scalar x ) { m_vec = XMVectorPermute<4,1,2,3>(m_vec, x); }
		INLINE void SetY( Scalar y ) { m_vec = XMVectorPermute<0,5,2,3>(m_vec, y); }
		INLINE void SetZ( Scalar z ) { m_vec = XMVectorPermute<0,1,6,3>(m_vec, z); }
		INLINE void SetW( Scalar w ) { m_vec = XMVectorPermute<0,1,2,7>(m_vec, w); }

		INLINE Quaternion operator~ ( void ) const { return Quaternion(XMQuaternionConjugate(m_vec)); }
		INLINE Quaternion operator- ( void ) const { return Quaternion(XMVectorNegate(m_vec)); }

		INLINE Quaternion operator+ ( Quaternion q2 ) const { return Quaternion(XMVectorAdd(m_vec, q2)); }
		INLINE Quaternion operator- ( Quaternion q2 ) const { return Quaternion(XMVectorSubtract(m_vec, q2)); }
		INLINE Quaternion operator+ ( Vector4 v2 ) const { return Quaternion(XMVectorAdd(m_vec, v2)); }
		INLINE Quaternion operator- ( Vector4 v2 ) const { return Quaternion(XMVectorSubtract(m_vec, v2)); }
		INLINE Quaternion operator+ ( Scalar v2 ) const { return Quaternion(XMVectorAdd(m_vec, v2)); }
		INLINE Quaternion operator- ( Scalar v2 ) const { return Quaternion(XMVectorSubtract(m_vec, v2)); }

		INLINE Quaternion operator* ( Quaternion rhs ) const { return Quaternion(XMQuaternionMultiply(rhs, m_vec)); }
		INLINE Vector3 operator* ( Vector3 rhs ) const { return Vector3(XMVector3Rotate(rhs, m_vec)); }

		INLINE Quaternion& operator= ( Quaternion rhs ) { m_vec = rhs; return *this; }
		INLINE Quaternion& operator*= ( Quaternion rhs ) { *this = *this * rhs; return *this; }

		INLINE Quaternion operator* ( Vector4 v2 ) const { return Quaternion(XMVectorMultiply(m_vec, v2)); }
		INLINE Quaternion operator/ ( Vector4 v2 ) const { return Quaternion(XMVectorDivide(m_vec, v2)); }
		INLINE Quaternion operator* ( Scalar  v2 ) const { return *this / Vector4(v2); }
		INLINE Quaternion operator/ ( Scalar  v2 ) const { return *this / Vector4(v2); }
		INLINE Quaternion operator* ( float   v2 ) const { return *this * Scalar(v2); }
		INLINE Quaternion operator/ ( float   v2 ) const { return *this / Scalar(v2); }

		INLINE Quaternion& operator/= ( Scalar v2 ) { *this = *this / v2; return *this; }

		Vector3 Euler( void ) const;

	protected:
		XMVECTOR m_vec;
	};

	// Returns a quaternion such that q*start = dest
	Quaternion RotationBetweenVectors( Vector3 start, Vector3 dest );

    INLINE std::ostream& operator<<( std::ostream& os, const Quaternion& value )
    {
        return os << "(" << float( value.GetX() ) << ", " << float( value.GetY() ) << ", " << float( value.GetZ() ) << ", " << float( value.GetW() ) << ")";
    }
}
