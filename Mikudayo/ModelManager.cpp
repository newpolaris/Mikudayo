#include "stdafx.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "ModelManager.h"
#include "IModel.h"
#include "PmxModel.h"
#include "PmxInstant.h"
#include "SkydomeModel.h"
#include "ModelAssimp.h"
#include "RenderPipelineManager.h"

namespace ModelManager {
    std::map<std::wstring, std::shared_ptr<IModel>> m_Models;
} // namespace ModelManager {

void ModelManager::Initialize()
{
    RenderPipelineManager::Initialize();
    PmxModel::Initialize();
    BaseModel::Initialize();
    SkydomeModel::Initialize();
}

void ModelManager::Shutdown()
{
    m_Models.clear();
    PmxModel::Shutdown();
    BaseModel::Shutdown();
    SkydomeModel::Shutdown();
    RenderPipelineManager::Shutdown();
}

ModelType GetModelType( const std::wstring& FileName )
{
    auto path = boost::filesystem::path( FileName );
    auto ext = boost::to_lower_copy( path.extension().generic_wstring() );
    if (ext == L".pmx")
        return kModelPMX;
    if (ext == L".pmd")
        return kModelPMX;
    return kModelDefault;
}

SceneNodePtr ModelManager::Load( const ModelInfo& info )
{
    ModelType type = info.Type;
    if (type == kModelUnknown)
        type = GetModelType( info.ModelFile );
    if (type == kModelPMX)
    {
        if (m_Models.count( info.ModelFile ) == 0) 
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
    if (type == kModelSkydome)
    {
        auto model = std::make_shared<SkydomeModel>();
        if (model->Load( info ))
            return model;
    }
    // Try using assimp
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