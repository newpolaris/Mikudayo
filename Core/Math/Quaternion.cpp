#pragma once

#include "pch.h"
#include "Quaternion.h"

using namespace Math;

// Use code from 'Wikipedia'
Vector3 Quaternion::Euler( void ) const
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

// Returns a quaternion such that q*start = dest
Quaternion Math::RotationBetweenVectors( Vector3 start, Vector3 dest )
{
    start = Normalize( start );
    dest = Normalize( dest );

    Scalar cosTheta = Dot( start, dest );

    Vector3 rotationAxis;

    if (cosTheta < -1 + 0.001f) {
        // special case when vectors in opposite directions :
        // there is no "ideal" rotation axis
        // So guess one; any will do as long as it's perpendicular to start
        // This implementation favors a rotation around the Up axis,
        // since it's often what you want to do.
        rotationAxis = Cross( Vector3( 0.0f, 0.0f, 1.0f ), start );
        if (LengthSquare( rotationAxis ) < 0.1f) // bad luck, they were parallel, try again!
            rotationAxis = Cross( Vector3( 1.0f, 0.0f, 0.0f ), start );

        rotationAxis = Normalize( rotationAxis );
        return Quaternion( rotationAxis, XM_PI );
    }

    // Implementation from Stan Melax's Game Programming Gems 1 article
    rotationAxis = Cross( start, dest );

    float s = sqrtf( (1.0f + cosTheta) * 2.f );
    float invs = 1 / s;

    rotationAxis *= Vector3( invs, invs, invs );
    return Quaternion( Vector4( rotationAxis, s * 0.5f ) );
}
