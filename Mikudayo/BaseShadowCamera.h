#pragma once

#include "Camera.h"

namespace Math
{
    class BaseShadowCamera : public BaseCamera
    {
    public:

        BaseShadowCamera() {}

        // Used to transform world space to texture space for shadow sampling
        const Matrix4& GetShadowMatrix() const { return m_ShadowMatrix; }

        void UpdateViewProjMatrix( const Matrix4& View, const Matrix4& Projection );

    private:

        Matrix4 m_ShadowMatrix;
    };
}
