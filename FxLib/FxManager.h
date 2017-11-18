#pragma once

#include <memory>

struct FxInfo
{
    std::string Name;
    std::wstring FilePath;
};

using FxTechniqueSetPtr = std::shared_ptr<class FxTechniqueSet>;

namespace FxManager
{
    void Initialize();
    void Shutdown();
    bool Load( const FxInfo& Fx );
    bool Load( const std::vector<FxInfo>& Fx );
    FxTechniqueSetPtr GetTechniques(const std::string& Fx);
}
