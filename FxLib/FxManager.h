#pragma once

struct FxInfo
{
    std::string Name;
    std::wstring FilePath;
};

class FxContainer;
namespace FxManager
{
    void Initialize();
    void Shutdown();
    FxContainer* GetFx( const std::string& Fx );
    void Load( const std::vector<FxInfo>& Fx );
}
