#pragma once

#include "Visitor.h"

class Material;
class OutlinePass : public Visitor
{
public:

    OutlinePass();
    bool Visit( const Material& material ) override;
};

