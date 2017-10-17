#pragma once

#include "RenderPass.h"

class IMaterial;
class OutlinePass : public RenderPass
{
public:

    OutlinePass();
    bool Visit( IMaterial& material ) override;
};

