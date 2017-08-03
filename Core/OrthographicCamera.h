#pragma once

#include "Camera.h"

namespace Math
{
    class OrthographicCamera : public BaseCamera
    {
    public:
        OrthographicCamera();
        void SetOrthographic( float minX, float maxX, float minY, float maxY, float nearClip, float farClip );
        void UpdateProjection( const Matrix4& proj ) { SetProjMatrix( proj ); }
    };
}