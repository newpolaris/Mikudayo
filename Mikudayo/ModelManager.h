#pragma once

#include <string>
#include "Model.h"

namespace ModelManager {
    void Initialize();
    void Shutdown();
    bool Load(const ModelInfo& Info);
    const Model& GetModel( const std::wstring& Name );
}
