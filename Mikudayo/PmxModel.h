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
        const std::vector<XMFLOAT3>& GetVertices( void ) const;
        const std::vector<uint32_t>& GetIndices( void ) const;
        bool LoadFromFile( const std::wstring& FilePath );
        void SetVertices( const std::vector<XMFLOAT3>& vertices );

    private:

        struct Private;
        std::shared_ptr<Private> m_Context;
    };
}