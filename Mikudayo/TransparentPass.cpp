#include "stdafx.h"
#include "TransparentPass.h"
#include "Material.h"

TransparentPass::TransparentPass()
{
}

bool TransparentPass::Visit( const Material& material )
{
    return material.IsTransparent();
}
