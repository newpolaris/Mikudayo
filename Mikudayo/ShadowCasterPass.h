#pragma once

#include "RenderPass.h"

class Material;
class ShadowCasterPass : public RenderPass
{
public:

    ShadowCasterPass();
    bool Visit( Material& material ) override;
};

