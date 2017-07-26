#pragma once

#include <vector>
#include "Camera.h"
#include "VectorMath.h"

using namespace Math;

namespace Graphics
{
    std::vector<Matrix4> SplitFrustum( const BaseCamera& Camera, const Matrix4& ShadowView );
}