#pragma once

#include "Quaternion.h"
#include "Vector.h"

namespace Math
{
	// Returns a quaternion such that q*start = dest
	Quaternion RotationBetweenVectors( Vector3 start, Vector3 dest );
}
