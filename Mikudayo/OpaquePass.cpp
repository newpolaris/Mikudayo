#include "stdafx.h"
#include "OpaquePass.h"
#include "Material.h"

OpaquePass::OpaquePass( RenderQueue Queue ) : RenderPass( Queue )
{
}

bool OpaquePass::Enable( IMaterial& material )
{
    return !material.IsTransparent();
}
