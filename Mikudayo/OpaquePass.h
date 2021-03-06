#pragma once

#include "RenderPass.h"

class IMaterial;
class OpaquePass : public RenderPass 
{
public:

    OpaquePass( RenderQueue Queue );
    bool Enable( IMaterial& material ) override;
};

