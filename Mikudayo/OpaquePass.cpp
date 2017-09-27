#include "stdafx.h"
#include "OpaquePass.h"
#include "Material.h"

OpaquePass::OpaquePass()
{
}

bool OpaquePass::Visit( const Material& material )
{
    return !material.IsTransparent();
}
