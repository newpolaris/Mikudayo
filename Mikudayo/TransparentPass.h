#pragma once

#include "RenderPass.h"

class Material;
class TransparentPass : public RenderPass
{
public:

    TransparentPass();
    bool Visit( Material& material ) override;
};

