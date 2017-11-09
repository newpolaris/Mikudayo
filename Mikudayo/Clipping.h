#pragma once

#include <vector>
#include "Math/Vector.h"

namespace Math
{
    class BaseCamera;
    class BoundingBox;
    class BoundingFrustum;
    using VecPoint = std::vector<Math::Vector3>;
    using PolyObject = std::vector<VecPoint>;

    void calcFocusedLightVolumePoints( VecPoint& points, const Vector3& lightDir,
        const BoundingFrustum& worldFrustum, const BoundingBox& sceneAABox );
}
