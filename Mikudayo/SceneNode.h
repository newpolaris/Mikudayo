#pragma once

#include <memory>
#include <vector>

class GraphicsContext;

class SceneNode : public std::enable_shared_from_this<SceneNode>
{
public:

    SceneNode();

    virtual void AddChild( std::shared_ptr<SceneNode> pNode );
    virtual void Update( float Delta );
    virtual void Render( GraphicsContext& Context );

protected:

    typedef std::vector< std::shared_ptr<SceneNode> > NodeList;
    typedef std::multimap< std::string, std::shared_ptr<SceneNode> > NodeNameMap;

    NodeList m_Children;
    NodeNameMap m_ChildrenByName;
};
