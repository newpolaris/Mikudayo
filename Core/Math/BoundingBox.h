#pragma once

#pragma once

#include <array>
#include "VectorMath.h"

namespace Math
{
    class BoundingBox
    {
    public:
        BoundingBox( Vector3 MinVec, Vector3 MaxVec ) : m_Min( MinVec ), m_Max( MaxVec )
        {
        }

        std::array<Vector3, 8> GetCorners( void ) const;

        Vector3 m_Min;
        Vector3 m_Max;
    };

    inline std::array<Vector3, 8> BoundingBox::GetCorners( void ) const
    {
        float x = m_Min.GetX(), y = m_Min.GetY(), z = m_Min.GetZ();
        float lx = m_Max.GetX(), ly = m_Max.GetY(), lz = m_Max.GetZ();

        return {
            Vector3( x, y, lz), // kNearLowerLeft
            Vector3( x, ly, lz), // kNearUpperLeft
            Vector3( lx, y, lz), // kNearLowerRight
            Vector3( lx, ly, lz), // kNearUpperRight,

            Vector3( x, y, z), // kFarLowerLeft
            Vector3( x, ly, z), // kFarUpperLeft
            Vector3( lx, y, z), // kFarLowerRight
            Vector3( lx, ly, z)  // kFarUpperRight
        };
    }
}