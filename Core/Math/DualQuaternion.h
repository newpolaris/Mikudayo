#pragma once

#include "Transform.h"

namespace Math
{
    class DualQuaternion
    {
    public:
        INLINE DualQuaternion() {}
        DualQuaternion( OrthogonalTransform form );

    private:
        Quaternion m_Real;
        Quaternion m_Dual;
    };
}

