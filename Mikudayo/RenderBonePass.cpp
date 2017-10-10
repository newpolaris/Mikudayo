#include "stdafx.h"
#include "RenderBonePass.h"
#include "SceneNode.h"
#include "RenderArgs.h"

RenderBonePass::RenderBonePass()
{
}

bool RenderBonePass::Visit( SceneNode& node )
{
    if (m_RenderArgs != nullptr)
        node.RenderBone( m_RenderArgs->gfxContext, *this );
    return true;
}
