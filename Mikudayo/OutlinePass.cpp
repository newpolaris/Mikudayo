#include "stdafx.h"
#include "OutlinePass.h"
#include "Material.h"

OutlinePass::OutlinePass() : RenderPass( kRenderQueueOutline )
{
}

bool OutlinePass::Enable( IMaterial& material )
{
    return material.IsOutline();
}
