#pragma once

#include "RenderPass.h"

class Material;
class OutlinePass : public RenderPass
{
public:

    OutlinePass();
    bool Visit( Material& material ) override;
};

