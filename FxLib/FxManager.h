#pragma once

#include <memory>

struct FxInfo
{
    std::string Name;
    std::wstring FilePath;
};

class FxTechniqueSet;

namespace FxManager
{
    void Initialize();
    void Shutdown();
    std::shared_ptr<FxTechniqueSet> GetTechniques(const std::string& Fx);
    void Load( const std::vector<FxInfo>& Fx );
}
