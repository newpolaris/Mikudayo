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

namespace Forward 
{
    using namespace Math;

    void Initialize( void );
    void Render( std::shared_ptr<Scene>& scene, RenderArgs& args );
    void Shutdown( void );
    void UpdateLights( const Math::BaseCamera& C );
}
