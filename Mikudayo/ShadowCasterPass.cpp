#include "stdafx.h"
#include "ShadowCasterPass.h"
#include "Material.h"

ShadowCasterPass::ShadowCasterPass()
{
}

bool ShadowCasterPass::Visit( Material& material )
{
    return material.IsShadowCaster();
}
