#include "stdafx.h"
#include "OutlinePass.h"
#include "Material.h"

OutlinePass::OutlinePass() : RenderPass( kRenderQueueOutline )
{
}

bool OutlinePass::Visit( IMaterial& material )
{
    RenderPass::Visit( material );
    return material.IsOutline();
}
