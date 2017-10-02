#pragma once

class GraphicsContext;
class SceneNode;
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
    void Render( GraphicsContext& gfxContext, std::shared_ptr<SceneNode>& scene, RenderArgs* args );
    void Shutdown( void );
    void UpdateLights( const Math::Camera& C );
}
