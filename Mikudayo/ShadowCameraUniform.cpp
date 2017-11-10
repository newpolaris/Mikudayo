#include "stdafx.h"

#include "Scene.h"
#include "ShadowCameraUniform.h"

using namespace Math;

void ShadowCameraUniform::UpdateMatrix( const class Scene& Scene, const Vector3& LightDirection, const BaseCamera& Camera )
{
    Matrix4 view = Camera.GetViewMatrix();
    BoundingBox sceneAABox;
    for (auto& node : Scene) {
        auto box = node->GetBoundingBox();
        sceneAABox.Merge( box );
    }
    // caster and reciver use same sized box
    BoundingBox frustumAABB = sceneAABox;
    BoundingBox casterAABB = sceneAABox;
    // Toward light
    Vector3 lightVec = Normalize(-LightDirection);
    Vector3 eyeLightDir = view.Get3x3() * lightVec;

    frustumAABB = view * frustumAABB;
    casterAABB = view * casterAABB;

    //  light pt is "infinitely" far away from the view frustum.
    //  however, all that's really needed is to place it just outside of all shadow casters
    Vector3 frustumCenter = frustumAABB.GetCenter();
    float t;
    bool bIntersect = casterAABB.Intersect( &t, frustumCenter, eyeLightDir );
    ASSERT( bIntersect );
    if (!bIntersect)
        return;
    Vector3 lightPt = frustumCenter + 2.f*t*eyeLightDir;
    const Vector3 yAxis(0.f, 1.f, 0.f);
    const Vector3 forwardZ = Camera.GetForwardZ();
    Vector3 axis;
    if (fabsf( Dot( eyeLightDir, yAxis ) ) > 0.99f)
        axis = forwardZ;
    else
        axis = yAxis;

    Matrix4 lightView = MatrixLookAt( lightPt, frustumCenter, axis );
    frustumAABB = lightView * frustumAABB;
    casterAABB = lightView * casterAABB;

    //  use a small fudge factor for the near plane, to avoid some minor clipping artifacts
    Matrix4 lightProj = MatrixScaleTranslateToFit( frustumAABB, m_ReverseZ );

    lightView = lightView * view;

    UpdateViewProjMatrix( lightView, lightProj );
}
