#include "pch.h"
#include "Functions.h"

namespace Math
{
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
			return Quaternion( rotationAxis, 180.0f );
		}

		// Implementation from Stan Melax's Game Programming Gems 1 article
		rotationAxis = Cross( start, dest );

		float s = sqrtf( (1.0f + cosTheta) * 2.f );
		float invs = 1 / s;

		rotationAxis *= Vector3( invs, invs, invs );
		return Quaternion( Vector4( rotationAxis, s * 0.5f ) );
	}
}
