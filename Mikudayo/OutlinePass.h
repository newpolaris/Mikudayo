#pragma once

#include "RenderPass.h"

class IMaterial;
class OutlinePass : public RenderPass
{
public:

    OutlinePass();

    bool Enable( IMaterial& material ) override;
};

