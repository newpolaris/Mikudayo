#pragma once

#include "RenderPass.h"

class IMaterial;
class ShadowCasterPass : public RenderPass
{
public:

    ShadowCasterPass();

    virtual bool Enable( IMaterial& material ) override;
};

