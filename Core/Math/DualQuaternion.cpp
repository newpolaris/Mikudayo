#include "pch.h"
#include "DualQuaternion.h"

using namespace Math;

DualQuaternion::DualQuaternion( OrthogonalTransform form )
{
    m_Real = form.GetRotation();
    m_Dual = Quaternion( form.GetTranslation() * 0.5f ) * m_Real;
}
