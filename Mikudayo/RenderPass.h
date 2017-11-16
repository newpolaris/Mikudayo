#pragma once

#include "Visitor.h"
#include "RenderType.h"

class RenderArgs;
class SceneNode;
class IMesh;
class IMaterial;

class RenderPass : public Visitor
{
public:

    RenderPass( RenderQueue Queue = kRenderQueueEmpty );

    void SetRenderArgs( RenderArgs& args );
    void SetRenderQueue( RenderQueue queue );

    virtual bool Enable( SceneNode& node );
    virtual bool Enable( IMaterial& material );
    virtual bool Enable( IMesh& mesh );
    virtual bool Enable( IMesh& mesh, SceneNode& node );
    // Inherited from Visitor
    virtual bool Visit( SceneNode& node ) override;
    virtual bool Visit( IMaterial& material ) override;
    virtual bool Visit( IMesh& mesh ) override;
    virtual bool Visit( IMesh& mesh, SceneNode& node ) override;

    RenderQueue m_RenderQueue;
    RenderArgs* m_RenderArgs = nullptr;
};

inline bool RenderPass::Enable( IMaterial& )
{
    return true;
}
