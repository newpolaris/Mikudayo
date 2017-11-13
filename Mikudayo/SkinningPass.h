#pragma once
#include "RenderPass.h"

class SkinningPass : public RenderPass
{
public:

    SkinningPass();

    // Inherited from Visitor
    virtual bool Visit( SceneNode& node ) override;
};
