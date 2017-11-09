#pragma once

#include <memory>
#include <vector>

class GraphicsContext;
class RenderArgs;
class RenderPass;
class Visitor;
class btDynamicsWorld;

enum SceneNodeType
{
    kSceneNormal,
    kSceneMirror,
};

typedef std::shared_ptr<class SceneNode> SceneNodePtr;
class SceneNode : public std::enable_shared_from_this<SceneNode>
{
public:

    SceneNode();

    virtual void Accept( Visitor& visitor );
    virtual void AddChild( SceneNodePtr pNode );
    virtual void Render( GraphicsContext& gfxContext, Visitor& visitor );
    virtual void RenderBone( GraphicsContext& Context, Visitor& visitor );
    virtual void Update( float deltaT );
    virtual void UpdateAfterPhysics( float deltaT );
    virtual SceneNodeType GetType() const;
    virtual void SetType( SceneNodeType type );
    virtual Math::BoundingBox GetBoundingBox() const;
    virtual Math::Matrix4 GetTransform() const;
    virtual void SetTransform( const Math::Matrix4& transform );

protected:

    typedef std::vector<SceneNodePtr> NodeList;
    typedef std::multimap<std::string, SceneNodePtr> NodeNameMap;

    SceneNodeType m_NodeType;
    RenderArgs* m_RenderArgs;
    NodeList m_Children;
    NodeNameMap m_ChildrenByName;
};
