#pragma once

#include "pch.h"
#include "Quaternion.h"

using namespace Math;

// 
// Wikipedia
//
Vector3 Quaternion::toEuler( void ) const 
{
	XMFLOAT4 q;
	DirectX::XMStoreFloat4( &q, *this );
	float ysqr = q.y * q.y;

	// roll (x-axis rotation)
	float t0 = +2.0f * (q.w * q.x + q.y * q.z);
	float t1 = +1.0f - 2.0f * (q.x * q.x + ysqr);
	float roll = std::atan2(t0, t1);

	// pitch (y-axis rotation)
	float t2 = +2.0f * (q.w * q.y - q.z * q.x);
	t2 = t2 > 1.0f ? 1.0f : t2;
	t2 = t2 < -1.0f ? -1.0f : t2;
	float pitch = std::asin(t2);

	// yaw (z-axis rotation)
	float t3 = +2.0f * (q.w * q.z + q.x * q.y);
	float t4 = +1.0f - 2.0f * (ysqr + q.z * q.z);  
	float yaw = std::atan2(t3, t4);

	return Vector3( roll, pitch, yaw );
}
