#pragma once

#include "pch.h"
#include "Quaternion.h"

using namespace Math;

// 
// http://www.euclideanspace.com/maths/geometry/rotations/euler/
//
Vector3 Quaternion::toEuler( void ) const 
{
	XMFLOAT4 q1;
	DirectX::XMStoreFloat4( &q1, *this );

	float test = q1.x*q1.y + q1.z*q1.w;
	float heading, attitude, bank;
	// singularity at north pole
	if (test > 0.499) {
		heading = 2 * atan2( q1.x, q1.w );
		attitude = XM_PI / 2;
		bank = 0;
		return Vector3( bank, heading, attitude );
	}
	if (test < -0.499) { // singularity at south pole
		heading = -2 * atan2( q1.x, q1.w );
		attitude = -XM_PI / 2;
		bank = 0;
		return Vector3( bank, heading, attitude );
	}
	float sqx = q1.x*q1.x;
	float sqy = q1.y*q1.y;
	float sqz = q1.z*q1.z;
	heading = atan2( 2 * q1.y*q1.w - 2 * q1.x*q1.z, 1 - 2 * sqy - 2 * sqz );
	attitude = asin( 2 * test );
	bank = atan2( 2 * q1.x*q1.w - 2 * q1.y*q1.z, 1 - 2 * sqx - 2 * sqz );

	return Vector3( bank, heading, attitude );
}
