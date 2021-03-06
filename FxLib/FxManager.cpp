#include "stdafx.h"
#include "FxManager.h"
#include "FxContainer.h"
#include "FxTechniqueSet.h"

namespace {
    std::map<std::string, std::shared_ptr<FxContainer>> m_FxList;
    std::map<std::string, std::shared_ptr<FxTechniqueSet>> m_FxTechniques;
}

void FxManager::Initialize()
{
}

void FxManager::Shutdown()
{
    m_FxList.clear();
}

FxTechniqueSetPtr FxManager::GetTechniques( const std::string& Fx )
{
    auto it = m_FxTechniques.find(Fx);
    if (it == m_FxTechniques.end())
    {
        auto fx = m_FxList.find(Fx);
        if (fx == m_FxList.end())
            return nullptr;
        auto set = std::make_shared<FxTechniqueSet>(fx->second);
        m_FxTechniques.insert({ Fx, set });
        return set;
    }
    return it->second;
}

bool FxManager::Load( const FxInfo& Fx )
{
    auto cont = std::make_shared<FxContainer>( Fx.FilePath );
    if (!cont->Load())
        return false;
    m_FxList[Fx.Name].swap( cont );
    return true;
}

bool FxManager::Load( const std::vector<FxInfo>& Fx )
{
    for (auto& fx : Fx)
    {
        auto cont = std::make_shared<FxContainer>( fx.FilePath );
        if (!cont->Load())
            return false;
        m_FxList[fx.Name].swap( cont );
    }
    return true;
}
