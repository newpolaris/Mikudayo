#include "stdafx.h"
#include "OutlinePass.h"
#include "Material.h"

OutlinePass::OutlinePass()
{
}

bool OutlinePass::Visit( Material& material )
{
    return material.IsOutline();
}
