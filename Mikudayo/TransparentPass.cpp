#include "stdafx.h"
#include "TransparentPass.h"
#include "Material.h"

TransparentPass::TransparentPass() : RenderPass( kRenderQueueTransparent )
{
}

bool TransparentPass::Enable( IMaterial& material )
{
    return material.IsTransparent();
}
