#pragma once

#include "Visitor.h"

class Material;
class TransparentPass : public Visitor
{
public:

    TransparentPass();
    bool Visit( const Material& material ) override;
};

