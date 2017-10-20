#include "stdafx.h"
#include "ModelManager.h"
#include "Model.h"
#include "PmxModel.h"
#include "BaseModel.h"

namespace ModelManager {
    std::map<std::wstring, std::shared_ptr<IModel>> m_Models;
} // namespace ModelManager {

void ModelManager::Initialize()
{
    PmxModel::Initialize();
    BaseModel::Initialize();
}

void ModelManager::Shutdown()
{
    m_Models.clear();
    PmxModel::Shutdown();
    BaseModel::Shutdown();
}

bool ModelManager::Load( const ModelInfo& Info )
{
    std::shared_ptr<IModel> model;
    if (Info.Type == kModelPMX)
        model = std::make_shared<PmxModel>();
    else if (Info.Type == kModelDefault)
        model = std::make_shared<BaseModel>();
    if (!model->Load( Info ))
        return false;
    m_Models[Info.Name] = model;
    return true;
}

IModel& ModelManager::GetModel( const std::wstring& Name )
{
    ASSERT(m_Models.count(Name) > 0);
    return *m_Models[Name];
}
