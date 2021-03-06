#include "stdafx.h"
#include "SceneNode.h"
#include "Visitor.h"

using namespace Math;

SceneNode::SceneNode() : m_RenderArgs(nullptr), m_NodeType(kSceneNormal)
{
}

void SceneNode::Accept( Visitor& visitor )
{
    visitor.Visit( *this );
    for (auto child : m_Children)
        child->Accept( visitor );
}

void SceneNode::AddChild( SceneNodePtr pNode )
{
    m_Children.push_back( pNode );
}

void SceneNode::Render( GraphicsContext& gfxContext, Visitor& visitor )
{
    (gfxContext), (visitor);
}

void SceneNode::RenderBone( GraphicsContext& gfxContext, Visitor& visitor )
{
    (gfxContext), (visitor);
}

void SceneNode::Skinning( GraphicsContext& gfxContext, Visitor& visitor )
{
    (gfxContext), (visitor);
}

void SceneNode::Update( float deltaT )
{
    (deltaT);
}

void SceneNode::UpdateAfterPhysics( float deltaT )
{
    (deltaT);
}

SceneNodeType SceneNode::GetType() const
{
    return m_NodeType;
}

void SceneNode::SetType(SceneNodeType type)
{
    m_NodeType = type;
}

BoundingBox SceneNode::GetBoundingBox() const
{
    return BoundingBox();
}

AffineTransform SceneNode::GetTransform() const
{
    return AffineTransform(kIdentity);
}

void SceneNode::SetTransform( const Math::AffineTransform& )
{
}
