#pragma once

#include "RenderPass.h"

class IMaterial;
class TransparentPass : public RenderPass
{
public:

    TransparentPass();

    virtual bool Enable( IMaterial& material ) override;
};

