#include "stdafx.h"
#include "TransparentPass.h"
#include "Material.h"

TransparentPass::TransparentPass() : RenderPass( kRenderQueueTransparent )
{
}

bool TransparentPass::Visit( IMaterial& material )
{
    RenderPass::Visit( material );
    return material.IsTransparent();
}
