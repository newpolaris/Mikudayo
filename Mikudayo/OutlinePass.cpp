#include "stdafx.h"
#include "OutlinePass.h"
#include "Material.h"

OutlinePass::OutlinePass()
{
}

bool OutlinePass::Visit( const Material& material )
{
    return material.IsOutline();
}
