#include "stdafx.h"
#include "Scene.h"
#include "RenderPass.h"

class UpdatePass : public Visitor
{
public:
    UpdatePass( float DeltaT ) : m_DeltaT( DeltaT ) {}
    bool Visit( SceneNode& node ) override {
        node.Update( m_DeltaT );
        return true;
    }
    float m_DeltaT;
};

void Scene::UpdateScene( float Delta )
{
    UpdatePass updatePass( Delta );
    Accept( updatePass );
}

void Scene::Render( RenderPass& renderPass, RenderArgs& args )
{
    renderPass.SetRenderArgs( args );
    Accept( renderPass );
}

