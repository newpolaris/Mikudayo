#pragma once

#include "RenderPass.h"

class Material;
class OpaquePass : public RenderPass 
{
public:

    OpaquePass();
    bool Visit( Material& material ) override;
};

