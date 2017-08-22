#pragma once

#include <memory>
#include "IRenderObject.h"

class GraphicsContext;

namespace boost {
namespace filesystem {
    class path;
}
}
namespace Utility {
    using ArchivePtr = std::shared_ptr<class Archive>;
}
namespace Graphics
{
    enum eModelType
    {
        kModelPMD = 0,
        kModelPMX,
        kModelGRD,
        kModelMAX
    };
    using Utility::ArchivePtr;
    using Path = boost::filesystem::path;
    class IModel : public IRenderObject
    {
    public:
        virtual void Draw( GraphicsContext& gfxContext, eObjectFilter Filter ) = 0;
        virtual void Update( float deltaT ) = 0;
        virtual bool LoadModel( ArchivePtr& Archive, Path& FilePath ) = 0;
        virtual bool LoadMotion( const std::wstring& Motion ) = 0;
        virtual Math::BoundingBox GetBoundingBox() = 0;
    };
}
