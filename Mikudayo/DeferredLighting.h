#pragma once

namespace Math
{
    class Vector3;
    class Matrix4;
    class Camera;
}
using namespace Math;
struct LightData
{
    Vector4 PositionWS;
    Vector4 PositionVS;
    Vector3 DirectionWS;
    Vector3 DirectionVS;
    Color Color;
    float Range;
    uint32_t Type;
    float SpotlightAngle;
};



namespace Lighting
{
    enum { MaxLights = 128 };

    extern LightData m_LightData[MaxLights];
    extern StructuredBuffer m_LightBuffer;

    void CreateRandomLights(const Math::Vector3 minBound, const Math::Vector3 maxBound);

    void Initialize( void );
    void Shutdown( void );
    void UpdateLights( const Math::Camera& C );
}
