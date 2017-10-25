#pragma once

#include <memory>
#include "SceneNode.h"

using ScenePtr = std::shared_ptr<class Scene>;
class Scene : public SceneNode
{
public:

    void UpdateScene( float Delta );
    void UpdateSceneAfterPhysics( float Delta );
    void Render( RenderPass& renderPass, RenderArgs& args );

};
