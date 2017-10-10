#include "stdafx.h"
#include "OpaquePass.h"
#include "Material.h"

OpaquePass::OpaquePass()
{
}

bool OpaquePass::Visit( Material& material )
{
    return !material.IsTransparent();
}
