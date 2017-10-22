#pragma once

#include "ModelAssimp.h"

class ModelAccessory : public AssimpModel
{
public:

    virtual bool Load( const ModelInfo& info ) override;

private:

};

