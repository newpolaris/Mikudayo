#pragma once

#include <memory>

class SceneNode : public std::enable_shared_from_this<SceneNode>
{
public:

    SceneNode();
    void AddChild(std::shared_ptr<SceneNode> pNode);
    void Update();

protected:
    std::vector<SceneNode> m_Child;
};
