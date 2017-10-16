#include "stdafx.h"
#include "ShadowCasterPass.h"
#include "Material.h"

ShadowCasterPass::ShadowCasterPass() : RenderPass( kRenderQueueShadow )
{
}

bool ShadowCasterPass::Visit( Material& material )
{
    RenderPass::Visit( material );
    return material.IsShadowCaster();
}
