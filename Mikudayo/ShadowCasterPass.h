#pragma once

#include "Visitor.h"

class Material;
class ShadowCasterPass : public Visitor
{
public:

    ShadowCasterPass();
    bool Visit( const Material& material ) override;
};

