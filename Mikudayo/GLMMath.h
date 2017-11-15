#pragma once

#pragma warning(push)
#pragma warning(disable: 4201)
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#pragma warning(pop)

#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "Math/DualQuaternion.h"

namespace glm {
    inline Math::Vector3 Convert( const glm::vec3& v )
    {
        return Math::Vector3( v.x, v.y, v.z );
    }

    inline glm::vec3 Convert( const Math::Vector3& v )
    {
        return glm::vec3( v.GetX(), v.GetY(), v.GetZ() );
    }

    inline glm::quat Convert( const Math::Quaternion& q )
    {
        return glm::quat( q.GetW(), q.GetX(), q.GetY(), q.GetZ() );
    }

    inline Math::Quaternion Convert( const glm::quat& q )
    {
        return Math::Quaternion( q.x, q.y, q.z, q.w );
    }

    inline Math::DualQuaternion Convert( const glm::dualquat& dq )
    {
        Math::DualQuaternion q;
        q.Real = Convert( dq.real );
        q.Dual = Convert( dq.dual );
        return q;
    }

    inline glm::dualquat Convert( const Math::DualQuaternion& dq )
    {
        glm::dualquat q;
        q.real = glm::Convert( dq.Real );
        q.dual = glm::Convert( dq.Dual );
        return q;
    }

}
