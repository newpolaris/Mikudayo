#pragma once

#include "RenderPass.h"

class IMaterial;
class OpaquePass : public RenderPass 
{
public:

    OpaquePass( RenderQueue Queue );
    bool Visit( IMaterial& material ) override;
};

