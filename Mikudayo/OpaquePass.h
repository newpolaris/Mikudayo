#pragma once

#include "RenderPass.h"

class Material;
class OpaquePass : public RenderPass 
{
public:

    OpaquePass( RenderQueue Queue );
    bool Visit( Material& material ) override;
};

