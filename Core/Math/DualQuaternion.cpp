#include "pch.h"
#include "DualQuaternion.h"

#define USE_GLM_METHOD 1

using namespace Math;

DualQuaternion::DualQuaternion()
{ 
    Real = Quaternion();
    Dual = Quaternion( 0, 0, 0, 0 );
}

DualQuaternion::DualQuaternion( Quaternion real, Quaternion dual ) :
    Real(real), Dual( dual ) 
{
}

DualQuaternion::DualQuaternion( OrthogonalTransform o ) :
    Real(o.GetRotation())
{
    Quaternion q = o.GetRotation();
    Vector3 p = o.GetTranslation();
    Dual = Quaternion(
        +0.5f * (p.GetX()*q.GetW()  + p.GetY()*q.GetZ() - p.GetZ()*q.GetY()),
        +0.5f * (-p.GetX()*q.GetZ() + p.GetY()*q.GetW() + p.GetZ()*q.GetX()),
        +0.5f * (p.GetX()*q.GetY()  - p.GetY()*q.GetX() + p.GetZ()*q.GetW()),
        -0.5f * (p.GetX()*q.GetX()  + p.GetY()*q.GetY() + p.GetZ()*q.GetZ()) );
}

// Use code from 'glm'
Vector3 DualQuaternion::Transform( const Vector3& v ) const
{
    // diff < 1e-5
#if !USE_GLM_METHOD
    Vector3 translate = (D*Real.GetW() - Vector3(Real)*Dual.GetW() + Cross(Vector3(Real), Vector3(Dual))) * 2.f;
    return Real * v + Vector3( translate );
#else
    Vector3 R( Real ), D( Dual );
    return (Cross( R, Cross( R, v ) + v * Real.GetW() + D ) + D*Real.GetW() - R * Dual.GetW()) * 2 + v;
#endif
}

Vector3 DualQuaternion::Rotate( const Vector3& v ) const
{
    return Vector3( Real * v );
}