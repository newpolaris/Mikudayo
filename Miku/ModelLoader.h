#pragma once

#include <memory>
#include "IModel.h"
#include "Math/Vector.h"

namespace Graphics {
    using namespace Math;

    class ModelLoader
    {
    public:

        ModelLoader( const std::wstring& Model, const std::wstring& Motion = L"", const Vector3& Position = Vector3(kZero) );
        std::shared_ptr<IModel> Load();

    protected:

        std::shared_ptr<IModel> LoadModel( const std::wstring& Model );

        std::wstring m_Model;
        std::wstring m_Motion;
        Vector3 m_Position;
        bool m_bRightHand;
    };
} // namespace Graphics
