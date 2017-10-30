#include "pch.h"
#include "BoundingBox.h"

using namespace Math;

void BoundingBox::Merge( const Vector3& vec )
{
    m_Min = Min( m_Min, vec );
    m_Max = Max( m_Max, vec );
}
