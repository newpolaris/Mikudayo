#pragma once

#include "Math/Matrix4.h"
#include "Math/Vector.h"
#include "Math/Transform.h"
#include "Math/Functions.inl"
#define BT_NO_SIMD_OPERATOR_OVERLOADS 1
#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btTransform.h"

namespace Math
{
    inline btTransform Convert(const AffineTransform& Trans )
    {
        AffineTransform trans(Transpose(Trans.GetBasis()), Trans.GetTranslation());
        return *reinterpret_cast<const btTransform*>(&trans);
    }

    inline AffineTransform Convert(const btTransform& btTrans )
    {
        btTransform trans(btTrans.getBasis().transpose(), btTrans.getOrigin());
        return *reinterpret_cast<const AffineTransform*>(&trans);
    }

    inline Vector3 Convert(const btVector3& vector )
    {
        return *reinterpret_cast<const Vector3*>(&vector);
    }

    inline btVector3 Convert(const Vector3& vector )
    {
        return *reinterpret_cast<const btVector3*>(&vector);
    }

    inline Vector4 Convert(const btVector4& vector )
    {
        return *reinterpret_cast<const Vector4*>(&vector);
    }

    inline btVector4 Convert(const Vector4& vector )
    {
        return *reinterpret_cast<const btVector4*>(&vector);
    }

    inline Quaternion Convert( const btQuaternion& quat )
    {
        return *reinterpret_cast<const Quaternion*>(&quat);
    }

    inline btQuaternion Convert( const Quaternion& quat )
    {
        return *reinterpret_cast<const btQuaternion*>(&quat);
    }
}