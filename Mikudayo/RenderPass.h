#pragma once

#include "Visitor.h"

class RenderArgs;
class SceneNode;
class Mesh;
class Material;

class RenderPass : public Visitor
{
public:

    void SetRenderArgs( RenderArgs& args );

    // Inherited from Visitor
    virtual bool Visit( SceneNode& node ) override;

    RenderArgs* m_RenderArgs = nullptr;
};
