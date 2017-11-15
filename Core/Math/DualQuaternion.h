#pragma once

#include "Transform.h"
#include "Quaternion.h"

namespace Math
{
    class DualQuaternion
    {
    public:
        DualQuaternion();
        DualQuaternion( OrthogonalTransform form );
        explicit DualQuaternion( Quaternion real, Quaternion dual );

        Vector3 Transform( const Vector3& position ) const;
        Vector3 Rotate( const Vector3& vector ) const;

        INLINE DualQuaternion operator~ ( void ) const;
        INLINE DualQuaternion operator- ( void ) const { return DualQuaternion( -Real, -Dual ); }
        INLINE DualQuaternion operator+ ( DualQuaternion dq ) const { return DualQuaternion( Real + dq.Real, Dual + dq.Dual ); } 
		INLINE DualQuaternion operator- ( DualQuaternion dq ) const { return DualQuaternion( Real - dq.Real, Dual - dq.Dual ); }
        INLINE DualQuaternion operator+ ( Scalar dq ) const { return DualQuaternion( Real + dq, Dual + dq ); }
		INLINE DualQuaternion operator- ( Scalar dq ) const { return DualQuaternion( Real - dq, Dual - dq ); }

		INLINE DualQuaternion operator* ( Vector4 v2 ) const { return DualQuaternion( Real * v2, Dual * v2 ); }
		INLINE DualQuaternion operator/ ( Vector4 v2 ) const { return DualQuaternion( Real / v2, Dual / v2 ); }
		INLINE DualQuaternion operator* ( Scalar  v2 ) const { return *this * Vector4(v2); }
		INLINE DualQuaternion operator/ ( Scalar  v2 ) const { return *this / Vector4(v2); }
		INLINE DualQuaternion operator* ( float   v2 ) const { return *this * Scalar(v2); }
		INLINE DualQuaternion operator/ ( float   v2 ) const { return *this / Scalar(v2); }

        Quaternion Real;
        Quaternion Dual;
    };

    INLINE DualQuaternion DualQuaternion::operator~ ( void ) const
    {
        return DualQuaternion( ~Real, ~Dual );
    }
}

