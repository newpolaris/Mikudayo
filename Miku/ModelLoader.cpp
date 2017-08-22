#include "ModelLoader.h"

#include <boost/algorithm/string.hpp>
#include "Archive.h"
#include "Pmd/Model.h"
#include "Pmx/Model.h"

using namespace Graphics;

namespace {
    using namespace Utility;
}

ModelLoader::ModelLoader( const std::wstring& Model, const std::wstring& Motion, const Vector3& Position ) :
    m_Model(Model), m_Motion(Motion), m_Position(Position), m_bRightHand(true)
{
}

std::shared_ptr<IModel> ModelLoader::Load()
{
    std::shared_ptr<IModel> model = LoadModel( m_Model );
    if (model != nullptr)
        model->LoadMotion( m_Motion );
    return model;
}

const std::wstring PMDExtension( L".pmd" ), PMXExtension( L".pmx" );

std::wstring GetExtension( const Path& FilePath )
{
    return boost::to_lower_copy( FilePath.extension().wstring() );
}

std::shared_ptr<IModel> LoadModelHelper( std::shared_ptr<Archive> Archive, Path ModelPath )
{
    std::shared_ptr<IModel> model;
    auto extension = GetExtension(ModelPath);
    if ( extension == PMDExtension )
        model = std::make_shared<Graphics::Pmd::Model>();
    else if ( extension == PMXExtension )
        model = std::make_shared<Graphics::Pmx::Model>();
    if (!model->LoadModel( Archive, ModelPath ))
        return nullptr;
    return model;
}

std::shared_ptr<IModel> ModelLoader::LoadModel( const std::wstring& Model )
{
    if (isZip( Model ))
    {
        auto archive = std::make_shared<ZipArchive>( Model );
        auto files = archive->GetFileList();
        for (auto& file : files)
        {
            auto extension = GetExtension(file);
            if ( extension == PMDExtension || extension == PMXExtension )
            {
                Path excludeRoot( ++file.begin(), file.end() );
                return LoadModelHelper(archive, excludeRoot);
                break;
            }
        }
        return nullptr;
    }
    else
    {
        Path FilePath( Model );
        auto archive = std::make_shared<RelativeFile>( FilePath.parent_path() );
        return LoadModelHelper( archive, FilePath.filename() );
    }
}
