#pragma once

class GraphicsContext;
class btDynamicsWorld;

namespace Math
{
    class Matrix4;
}

namespace Physics
{
	extern btDynamicsWorld* g_DynamicsWorld;

    void Initialize( void );
    void Shutdown( void );
    void Update( float deltaT );
    void Render( GraphicsContext& Context, const Math::Matrix4& ClipToWorld );
};
