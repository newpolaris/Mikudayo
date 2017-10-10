#include "stdafx.h"
#include "RenderPass.h"
#include "RenderArgs.h"
#include "SceneNode.h"

void RenderPass::SetRenderArgs( RenderArgs& args )
{
    m_RenderArgs = &args;
}

bool RenderPass::Visit( SceneNode& node )
{
    if (m_RenderArgs != nullptr)
        node.Render( m_RenderArgs->gfxContext, *this );
    return true;
}