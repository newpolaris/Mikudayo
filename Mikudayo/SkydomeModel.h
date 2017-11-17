#pragma once

#include "IModel.h"
#include "BaseModelTypes.h"
#include "BaseModel.h"

class SkydomeModel : public BaseModel
{
public:
    SkydomeModel();

    static void Initialize();
    static void Shutdown();

    virtual bool Load( const ModelInfo& info ) override;
};
