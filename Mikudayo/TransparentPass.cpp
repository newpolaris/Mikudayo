#include "stdafx.h"
#include "TransparentPass.h"
#include "Material.h"

TransparentPass::TransparentPass() : RenderPass( kRenderQueueTransparent )
{
}

bool TransparentPass::Visit( Material& material )
{
    RenderPass::Visit( material );
    return material.IsTransparent();
}
