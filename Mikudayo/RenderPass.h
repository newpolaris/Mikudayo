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

    virtual bool Enable( IMaterial& material );
    // Inherited from Visitor
    virtual bool Visit( SceneNode& node ) override;
    virtual bool Visit( IMaterial& material ) override;

    RenderQueue m_RenderQueue;
    RenderArgs* m_RenderArgs = nullptr;
};

inline bool RenderPass::Enable( IMaterial& )
{
    return true;
}