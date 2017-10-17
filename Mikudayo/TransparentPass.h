#pragma once

#include "RenderPass.h"

class IMaterial;
class TransparentPass : public RenderPass
{
public:

    TransparentPass();
    bool Visit( IMaterial& material ) override;
};

