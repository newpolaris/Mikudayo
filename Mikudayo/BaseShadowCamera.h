#pragma once

#include "Camera.h"

class BaseShadowCamera : public Math::BaseCamera
{
public:

    BaseShadowCamera() {}

    const Math::Matrix4& GetViewMatrix() const { return m_ViewMatrix; }
    const Math::Matrix4& GetProjMatrix() const { return m_ProjMatrix; }
    const Math::Matrix4& GetViewProjMatrix() const { return m_ViewProjMatrix; }

    // Used to transform world space to texture space for shadow sampling
    const Math::Matrix4& GetShadowMatrix() const { return m_ShadowMatrix; }

    void UpdateViewProjMatrix( const Math::Matrix4& View, const Math::Matrix4& Projection );

private:

    Math::Matrix4 m_ShadowMatrix;
};
