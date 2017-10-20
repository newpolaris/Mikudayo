#include "RenderPass.h"

class RenderBonePass : public RenderPass
{
public:

    RenderBonePass();

    // Inherited from Visitor
    virtual bool Visit( SceneNode& node ) override;
};