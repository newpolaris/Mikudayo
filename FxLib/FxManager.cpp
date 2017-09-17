#include "stdafx.h"
#include "FxManager.h"
#include "FxContainer.h"

namespace {
    std::map<std::string, std::shared_ptr<FxContainer>> m_FxList;
}

void FxManager::Initialize()
{
}

void FxManager::Shutdown()
{
    m_FxList.clear();
}

FxContainer* FxManager::GetFx( const std::string& Fx )
{
    auto it = m_FxList.find(Fx);
    if (it != m_FxList.end())
        return it->second.get();
    return nullptr;
}

void FxManager::Load( const std::vector<FxInfo>& Fx )
{
    for (auto& fx : Fx)
    {
        auto cont = std::make_shared<FxContainer>( fx.FilePath );
        if (cont->Load())
            m_FxList[fx.Name].swap(cont);
    }
}