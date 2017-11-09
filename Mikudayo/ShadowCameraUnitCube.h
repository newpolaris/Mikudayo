#pragma once

#include <vector>
#include <memory>
#include "BaseShadowCamera.h"

class ShadowCameraUnitCube : public BaseShadowCamera
{
public:

    ShadowCameraUnitCube() {}

    const Math::Matrix4& GetViewMatrix() const { return m_ViewMatrix; }
    const Math::Matrix4& GetProjMatrix() const { return m_ProjMatrix; }
    const Math::Matrix4& GetViewProjMatrix() const { return m_ViewProjMatrix; }

    // Used to transform world space to texture space for shadow sampling
    const Math::Matrix4& GetShadowMatrix() const { return m_ShadowMatrix; }

    void UpdateMatrix( const class Scene& Model, Math::Vector3 LightDirection, const BaseCamera& Camera );

private:

    Math::Matrix4 m_ShadowMatrix;
};