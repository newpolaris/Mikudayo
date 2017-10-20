#pragma once

class GraphicsContext;
class Scene;
class RenderArgs;

namespace Math
{
    class Vector3;
    class Matrix4;
    class Camera;
}

namespace Lighting
{
    using namespace Math;
    enum { MaxLights = 128 };
    enum class LightType : uint32_t
    {
        Point = 0,
        Spot = 1,
        Directional = 2
    };
    struct LightData
    {
        Vector4 PositionWS;
        Vector4 PositionVS;
        Vector3 DirectionWS;
        Vector3 DirectionVS;
        Color Color;
        float Range;
        LightType Type;
        float SpotlightAngle;
        float Intensity;
    };
    extern LightData m_LightData[MaxLights];
    extern StructuredBuffer m_LightBuffer;

    void CreateRandomLights(const Math::Vector3 minBound, const Math::Vector3 maxBound);

    void Initialize( void );
    void Render( std::shared_ptr<Scene>& scene, RenderArgs& args );
    void Shutdown( void );
    void UpdateLights( const Math::BaseCamera& C );
}
