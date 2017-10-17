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
    enum { MaxLights = 128 };

    extern StructuredBuffer m_LightBuffer;

    void CreateRandomLights(const Math::Vector3 minBound, const Math::Vector3 maxBound);

    void Initialize( void );
    void Render( std::shared_ptr<Scene>& scene, RenderArgs& args );
    void Shutdown( void );
    void UpdateLights( const Math::BaseCamera& C );
}
