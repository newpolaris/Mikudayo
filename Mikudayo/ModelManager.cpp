#include "stdafx.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "ModelManager.h"
#include "Model.h"
#include "PmxModel.h"
#include "PmxInstant.h"
#include "ModelAssimp.h"

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

ModelType GetModelType( const std::wstring& FileName )
{
    auto path = boost::filesystem::path( FileName );
    auto ext = boost::to_lower_copy( path.extension().generic_wstring() );
    if (ext == L"pmx")
        return kModelPMX;
    return kModelDefault;
}

SceneNodePtr ModelManager::Load( const ModelInfo& info )
{
    ModelType type = GetModelType( info.ModelFile );
    if (type == kModelPMX)
    {
        if (m_Models.count( info.ModelFile )) 
        {
            auto model = std::make_shared<PmxModel>();
            if (!model->Load( info ))
                return nullptr;
            m_Models[info.ModelFile] = model;
        }
        auto base = m_Models[info.ModelFile];
        auto model = std::make_shared<PmxInstant>(*base);
        if (!model->Load())
            return nullptr;
        model->LoadMotion( info.MotionFile );
        return model;
    }
    auto model = std::make_shared<AssimpModel>();
    if (model->Load( info ))
        return model;
    return nullptr;
}

SceneNodePtr ModelManager::Load( const std::wstring& FileName )
{
    ModelInfo info;
    info.ModelFile = FileName;
    return Load( info );
}