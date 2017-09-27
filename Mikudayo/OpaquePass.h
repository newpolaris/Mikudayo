#pragma once

#include "Visitor.h"

class Material;
class OpaquePass : public Visitor
{
public:

    OpaquePass();
    bool Visit( const Material& material ) override;
};

