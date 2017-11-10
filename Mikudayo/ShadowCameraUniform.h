#pragma once

#include <vector>
#include <memory>
#include "BaseShadowCamera.h"

// view-space uniform shadow maps
namespace Math
{
    class ShadowCameraUniform : public BaseShadowCamera
    {
    public:

        ShadowCameraUniform() {}

        void UpdateMatrix( const class Scene& Scene, const Vector3& LightDirection, const BaseCamera& Camera );

    private:

        Matrix4 m_ShadowMatrix;
    };
}