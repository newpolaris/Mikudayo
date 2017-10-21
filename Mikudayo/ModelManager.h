#pragma once

#include <memory>
#include <string>
#include "Model.h"
#include "SceneNode.h"

namespace ModelManager {
    void Initialize();
    void Shutdown();

    SceneNodePtr Load( const ModelInfo& Info );
    SceneNodePtr Load( const std::wstring& FileName );
}
