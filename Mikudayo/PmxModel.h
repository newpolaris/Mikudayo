#pragma once

#include <memory>
#include <string>

namespace Rendering
{
    class PmxModel
    {
    public:

        PmxModel();
        void Clear();
        void DrawColor( GraphicsContext& Context );
        bool LoadFromFile( const std::wstring& FilePath );

    private:

        struct Private;
        std::shared_ptr<Private> m_Context;
    };
}