#include "stdafx.h"
#include "ShadowCasterPass.h"
#include "Material.h"

ShadowCasterPass::ShadowCasterPass() : RenderPass( kRenderQueueShadow )
{
}

bool ShadowCasterPass::Enable( IMaterial& material )
{
    return material.IsShadowCaster();
}
