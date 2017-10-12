#pragma once

#include <memory>
#include <vector>

class GraphicsContext;
class RenderArgs;
class RenderPass;
class Visitor;
class btDynamicsWorld;

class SceneNode : public std::enable_shared_from_this<SceneNode>
{
public:

    SceneNode();

    virtual void Accept( Visitor& visitor );
    virtual void AddChild( std::shared_ptr<SceneNode> pNode );
    virtual void JoinWorld( btDynamicsWorld* world );
    virtual void LeaveWorld( btDynamicsWorld* world );
    virtual void Render( GraphicsContext& gfxContext, Visitor& visitor );
    virtual void RenderBone( GraphicsContext& Context, Visitor& visitor );
    virtual void Update( float deltaT );

protected:

    typedef std::vector< std::shared_ptr<SceneNode> > NodeList;
    typedef std::multimap< std::string, std::shared_ptr<SceneNode> > NodeNameMap;

    RenderArgs* m_RenderArgs;
    NodeList m_Children;
    NodeNameMap m_ChildrenByName;
};
