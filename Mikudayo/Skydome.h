#pragma once

#include "Scene.h"

class GraphicsContext;
class RenderArgs;

namespace Math
{
    class Vector3;
    class Matrix4;
    class Camera;
}

namespace Skydome
{
    void Initialize( void );
    void Shutdown( void );
    void Render( ScenePtr& scene, RenderArgs& args );
}
