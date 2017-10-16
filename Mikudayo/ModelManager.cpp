#include "stdafx.h"
#include "ModelManager.h"
#include "Model.h"
#include "PmxModel.h"

namespace ModelManager {
    std::map<std::wstring, std::shared_ptr<Model>> m_Models;
} // namespace ModelManager {

void ModelManager::Initialize()
{
    PmxModel::Initialize();
}

void ModelManager::Shutdown()
{
    m_Models.clear();
    PmxModel::Shutdown();
}

bool ModelManager::Load( const ModelInfo& Info )
{
    std::shared_ptr<PmxModel> model;
    if (Info.Type == kModelPMX)
        model = std::make_shared<PmxModel>();
    if (!model->Load( Info ))
        return false;
    m_Models[Info.Name] = model;
    return true;
}

Model& ModelManager::GetModel( const std::wstring& Name )
{
    ASSERT(m_Models.count(Name) > 0);
    return *m_Models[Name];
}
