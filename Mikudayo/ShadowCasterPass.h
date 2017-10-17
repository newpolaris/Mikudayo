#pragma once

#include "RenderPass.h"

class IMaterial;
class ShadowCasterPass : public RenderPass
{
public:

    ShadowCasterPass();
    bool Visit( IMaterial& material ) override;
};

