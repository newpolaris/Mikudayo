#pragma once

#include <memory>
#include <vector>

class GraphicsContext;
class RenderArgs;
class RenderPass;
class Visitor;
class btDynamicsWorld;

typedef std::shared_ptr<class SceneNode> SceneNodePtr;
class SceneNode : public std::enable_shared_from_this<SceneNode>
{
public:

    SceneNode();

    virtual void Accept( Visitor& visitor );
    virtual void AddChild( std::shared_ptr<SceneNode> pNode );
    virtual void Render( GraphicsContext& gfxContext, Visitor& visitor );
    virtual void RenderBone( GraphicsContext& Context, Visitor& visitor );
    virtual void Update( float deltaT );
    virtual void UpdateAfterPhysics( float deltaT );
    virtual Math::Matrix4 GetTransform() const;
    virtual void SetTransform( const Math::Matrix4& transform );

protected:

    typedef std::vector<std::shared_ptr<SceneNode>> NodeList;
    typedef std::multimap<std::string, std::shared_ptr<SceneNode>> NodeNameMap;

    RenderArgs* m_RenderArgs;
    NodeList m_Children;
    NodeNameMap m_ChildrenByName;
};
