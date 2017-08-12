#pragma once

#include <memory>

#define BT_NO_SIMD_OPERATOR_OVERLOADS 1
#include "LinearMath/btIDebugDraw.h"

class GraphicsPSO;
extern GraphicsPSO PSO;

class GraphicsContext;
namespace Math
{
    class Matrix4;
}

namespace BulletDebug
{
    extern void Initialize();
    extern void Shutdown();

    class DebugDraw : public btIDebugDraw
    {
    public:
        DebugDraw();
        void drawLine( const btVector3& from, const btVector3& to, const btVector3& color ) override;
        void drawContactPoint( const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color ) override;
        void reportErrorWarning( const char* warningString ) override;
        void draw3dText( const btVector3& location, const char* textString ) override;
        void setDebugMode( int debugMode ) override;
        int	getDebugMode() const override;
        void flush( GraphicsContext& UiContext, const Math::Matrix4& WorldToClip );

    protected:
        int m_DebugMode;

        struct Context;
        std::shared_ptr<Context> m_Context;
    };

    inline void DebugDraw::drawContactPoint( const btVector3& PointOnB, const btVector3 & normalOnB, btScalar distance, int lifeTime, const btVector3 & color )
    {
        // it seems the only property, no manual lifeTime handling is needed
        (lifeTime);

        drawLine( PointOnB, PointOnB + normalOnB*distance, color );
        btVector3 ncolor( 0, 0, 0 );
        drawLine( PointOnB, PointOnB + normalOnB*0.01f, ncolor );
    }

    inline void DebugDraw::setDebugMode( int debugMode )
    {
        m_DebugMode = debugMode;
    }

    inline int DebugDraw::getDebugMode() const
    {
        return m_DebugMode;
    }
}