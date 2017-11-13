#include "stdafx.h"
#include "SkinningPass.h"
#include "SceneNode.h"
#include "RenderArgs.h"

SkinningPass::SkinningPass() : RenderPass( kRenderQueueSkinning )
{
}

bool SkinningPass::Visit( SceneNode& node )
{
    if (m_RenderArgs != nullptr)
        node.Skinning( m_RenderArgs->gfxContext, *this );
    return true;
}
