#include "stdafx.h"

#include "Scene.h"
#include "ShadowCameraUnitCube.h"

using namespace Math;

void ShadowCameraUnitCube::UpdateMatrix( const Scene& scene, Vector3 lightDirection, const BaseCamera& camera )
{
#if 0
    Matrix4 view = camera.GetViewMatrix();

    Vector3 minVec( std::numeric_limits<float>::max() ), maxVec( std::numeric_limits<float>::lowest() );
    for (auto& model : scene)
    {
        auto bound = model->GetBoundingBox();
        minVec = Min(bound.GetMin(), minVec);
        maxVec = Max(bound.GetMax(), maxVec);
    }

    // caster and reciver use same sized box
    BoundingBox frustumAABB( minVec, maxVec );
    BoundingBox casterAABB( minVec, maxVec );
    Vector3 lightDir = Normalize(-lightDirection);
    Vector3 eyeLightDir = view.Get3x3() * lightDir;

    frustumAABB = view * frustumAABB;
    casterAABB = view * casterAABB;

    //  light pt is "infinitely" far away from the view frustum.
    //  however, all that's really needed is to place it just outside of all shadow casters
    Vector3 frustumCenter = frustumAABB.GetCenter();
    float t;
    casterAABB.Intersect( &t, frustumCenter, eyeLightDir );

    Vector3 lightPt = frustumCenter + 2.f*t*eyeLightDir;
    const Vector3 yAxis(0.f, 1.f, 0.f);
    const Vector3 zAxis(0.f, 0.f, 1.f);
    Vector3 axis;
    if (fabsf( Dot( eyeLightDir, yAxis ) ) > 0.99f)
        axis = zAxis;
    else
        axis = yAxis;

    Matrix4 lightView = MatrixLookAtLH( lightPt, frustumCenter, axis );
    frustumAABB = lightView * frustumAABB;
    casterAABB = lightView * casterAABB;

    //  use a small fudge factor for the near plane, to avoid some minor clipping artifacts
    Matrix4 lightProj = OrthographicMatrix(
        frustumAABB.m_Min.GetX(), frustumAABB.m_Max.GetX(),
        frustumAABB.m_Min.GetY(), frustumAABB.m_Max.GetY(),
        casterAABB.m_Min.GetZ(), frustumAABB.m_Max.GetZ(),
        m_ReverseZ );

    lightView = lightView * view;

    UpdateViewProjMatrix( lightView, lightProj );
#endif
}
