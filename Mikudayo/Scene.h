#pragma once

#include "SceneNode.h"

class Scene : public SceneNode
{
public:

    void UpdateScene( float Delta );
    void Render( RenderPass& renderPass, RenderArgs& args );

};
