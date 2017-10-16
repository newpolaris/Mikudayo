#include "stdafx.h"
#include "OpaquePass.h"
#include "Material.h"

OpaquePass::OpaquePass( RenderQueue Queue ) : RenderPass( Queue )
{
}

bool OpaquePass::Visit( Material& material )
{
    RenderPass::Visit( material );
    return !material.IsTransparent();
}
