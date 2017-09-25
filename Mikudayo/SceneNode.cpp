#include "stdafx.h"
#include "SceneNode.h"

SceneNode::SceneNode()
{
}

void SceneNode::AddChild( std::shared_ptr<SceneNode> pNode )
{
    m_Children.push_back( pNode );
}

void SceneNode::Update( float Delta )
{
    for (auto child : m_Children)
        child->Update( Delta );
}

void SceneNode::Render( GraphicsContext& Context )
{
    for (auto child : m_Children)
        child->Render( Context );
}
