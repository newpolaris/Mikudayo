#pragma once

#include <vector>
#include <memory>
#include "Camera.h"
#include "BaseShadowCamera.h"
#include "Clipping.h"

class Scene;
namespace Math
{
    class ShadowCameraLiSPSM : public BaseShadowCamera
    {
    public:

        ShadowCameraLiSPSM() {}

        void UpdateMatrix( const Scene& Model, Vector3 LightDirection, const BaseCamera& Camera );

    private:

        void CalcLispSMMtx( const Vector3& eyePos, const Vector3& lightDir, const Vector3& viewDir, const VecPoint& B );
        void CalcUniformShadowMtx( const Vector3& eyePos, const Vector3& lightDir, const Vector3& viewDir, const VecPoint& B );

        Math::Matrix4 m_ShadowMatrix;
    };
}
