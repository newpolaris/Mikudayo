#pragma once

#include "Visitor.h"
#include "RenderType.h"

class RenderArgs;
class SceneNode;
class Mesh;
class Material;

class RenderPass : public Visitor
{
public:

    RenderPass( RenderQueue Queue = kRenderQueueEmpty );

    void SetRenderArgs( RenderArgs& args );

    // Inherited from Visitor
    virtual bool Visit( SceneNode& node ) override;
    virtual bool Visit( Material& material ) override;

    RenderQueue m_RenderQueue;
    RenderArgs* m_RenderArgs = nullptr;
};
