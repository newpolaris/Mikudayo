//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "pch.h"
#include "ShadowCamera.h"
#include "Math/BoundingBox.h"

using namespace Math;
using namespace GameCore;

BoolVar RecenterCamera( "Graphics/Camera/Recenter", true );

void ShadowCamera::UpdateMatrix(
    Vector3 LightDirection, Vector3 ShadowCenter, Vector3 ShadowBounds,
    uint32_t BufferWidth, uint32_t BufferHeight, uint32_t BufferPrecision )
{
    // Converts world units to texel units so we can quantize the camera position to whole texel units
    Vector3 RcpDimensions = Recip(ShadowBounds);
    Vector3 QuantizeScale = Vector3((float)BufferWidth, (float)BufferHeight, (float)((1 << BufferPrecision) - 1)) * RcpDimensions;

    //
    // Recenter the camera at the quantized position
    //
    if (RecenterCamera)
    {
        // Transform to view space
        ShadowCenter = ~GetRotation() * ShadowCenter;
        // Scale to texel units, truncate fractional part, and scale back to world units
        ShadowCenter = Floor( ShadowCenter * QuantizeScale ) / QuantizeScale;
        // Transform back into world space
        ShadowCenter = GetRotation() * ShadowCenter;
    }
    SetPosition( ShadowCenter );

    SetLookDirection( LightDirection, Vector3(kYUnitVector) );
    SetProjMatrix( OrthographicMatrix(ShadowBounds.GetX(), ShadowBounds.GetY(), -ShadowBounds.GetZ(), ShadowBounds.GetZ(), m_ReverseZ) );

    Update();

    // Transform from clip space to texture space
    m_ShadowMatrix = Matrix4( AffineTransform( Matrix3::MakeScale( 0.5f, -0.5f, 1.0f ), Vector3(0.5f, 0.5f, 0.0f) ) ) * m_ViewProjMatrix;
    m_ShadowMatrix = m_ViewProjMatrix;
}

void ShadowCamera::UpdateMatrix( Vector3 LightVector, Vector3 ShadowCenter, Vector3 ShadowBounds, BaseCamera& camera )
{
    SetPosition( ShadowCenter );

    SetLookDirection( LightVector, Vector3(kYUnitVector) );
    SetProjMatrix( OrthographicMatrix(ShadowBounds.GetX(), ShadowBounds.GetY(), -ShadowBounds.GetZ(), ShadowBounds.GetZ(), m_ReverseZ) );
    SetProjMatrix( PerspectiveMatrix(XM_PI / 4, 1.0f, 10.f, 1000, m_ReverseZ ) );

	m_ViewMatrix = Matrix4(~m_CameraToWorld);
	m_ViewProjMatrix = m_ProjMatrix * m_ViewMatrix;

    Vector3 lightDir = Normalize(LightVector);
    Matrix4 invViewProj = Invert( camera.GetViewProjMatrix() );
    Vector3 cameraPos = camera.GetPosition();

    //  these are the limits specified by the physical camera
    //  gamma is the "tilt angle" between the light and the view direction.
    Vector3 viewDir = camera.GetForwardVec();
    float fCosGamma = Dot( lightDir, viewDir );
    ASSERT( fabsf( fCosGamma ) < 0.99f );
    Frustum corners = camera.GetWorldSpaceFrustum();
    FrustumCorner corner = corners.GetFrustumCorners();
    std::vector<Vector3> bodyB;
    std::copy( corner.begin(), corner.end(), std::back_inserter( bodyB ) );

    Matrix4 cameraView = camera.GetViewMatrix();
    const Vector3 backVector = -camera.GetForwardVec();

    auto calcUpVec = []( Vector3 viewDir, Vector3 lightDir ) {
        Vector3 left = Cross( lightDir, viewDir );
        return Normalize(Cross( left, lightDir ));
    };
    Vector3 up = calcUpVec( viewDir, lightDir );
    auto look = []( const Vector3 pos, const Vector3 dir, const Vector3 up ) {
        Vector3 dirN;
        Vector3 upN;
        Vector3 lftN;

        lftN = Cross( dir, up );
        lftN = Normalize( lftN );

        upN = Cross( lftN, dir );
        upN = Normalize( upN );
        dirN = Normalize( dir );

        Matrix4 lightSpaceBasis( lftN, upN, -dirN, Vector3( kZero ) );
        lightSpaceBasis = Transpose( lightSpaceBasis );
        Matrix4 lightView = lightSpaceBasis * Matrix4::MakeTranslate( -pos );
        return lightView;
    };

    auto look2 = []( const Vector3 pos, const Vector3 dir, const Vector3 up ) {
        Vector3 right = Normalize(Cross( up, dir ));
        Vector3 viewY = Normalize(Cross( dir, right ));
        Vector3 viewZ = Normalize( dir );

        Matrix3 basis = Transpose(Matrix3( right, viewY, viewZ ));
        return Matrix4( basis, basis * -pos );
    };

    Matrix4 lightView = look( cameraPos, lightDir, up );

    std::vector<Vector3> bodyCopyB = bodyB;
    for (auto& body : bodyB)
        body = lightView.Transform(body);

    Math::BoundingBox aabb( bodyB );
    Vector3 nearPt( kZero );
    nearPt = invViewProj.Transform( nearPt );

    Scalar nearDist = Length( nearPt - cameraPos );
    float sinGamma = sqrtf( 1.f - fCosGamma*fCosGamma );
    float factor = 1.0f / sinGamma;
    float z_n = factor * nearDist;
    float d = Abs( aabb.GetMax().GetY() - aabb.GetMin().GetY() );
    float z_f = z_n + d * sinGamma;
    float n = (z_n + Sqrt( z_f * z_n )) / sinGamma;
    float f = n + d;
    Vector3 pos = cameraPos + up * -(n - nearDist);
    lightView = look(pos, lightDir, up );

    float a, b;
    if (m_ReverseZ)
    {
        a = -n / (f - n);
        b = a * f;
    }
    else
    {
        a = (f+n) / (f-n);
        b = -2 * f*n / (f - n);
    }

    Matrix4 lispMatrix(
        Vector4( 1, 0, 0, 0 ),
        Vector4( 0, a, 0, 1 ),
        Vector4( 0, 0, 1, 0 ),
        Vector4( 0, b, 0, 0 ) );

    Matrix4 lightProjection = lispMatrix * lightView;

    for (auto& body : bodyCopyB)
        body = lightProjection.Transform(body);

    Math::BoundingBox aabbCopy( bodyCopyB );

    auto scaleTranslateToFit = [](const Vector3 min, const Vector3 max) {
        XMFLOAT3 vMin = XMFLOAT3(min.GetX(), min.GetY(), min.GetZ() );
        XMFLOAT3 vMax = XMFLOAT3(max.GetX(), max.GetY(), max.GetZ() );
        float tx = -(vMax.x + vMin.x) / (vMax.x - vMin.x),
            ty = - (vMax.y + vMin.y) / (vMax.y - vMin.y),
            tz = -(vMax.z + vMin.z) / (vMax.z - vMin.z);
        Matrix4 output(
            Vector4( 2 / (vMax.x - vMin.x), 0, 0, 0 ),
            Vector4( 0, 2 / (vMax.y - vMin.y), 0, 0 ),
            Vector4( 0, 0, 2 / (vMax.z - vMin.z), 0 ),
            Vector4( tx, ty, tz, 1 ) );
        return output;
    };
    lightProjection = scaleTranslateToFit( aabbCopy.GetMin(), aabbCopy.GetMax() );
    Matrix4 rh2lh = Matrix4::MakeScale( Vector3(1.f, 1.f, -1.f) );
    Matrix4 fitDxNDC(
        Vector4( 1, 0, 0, 0 ),
        Vector4( 0, 1, 0, 0 ),
        Vector4( 0, 0, 0.5, 0 ),
        Vector4( 0, 0, 0.5, 1 ) );

    m_ViewMatrix = lightView;
    m_ProjMatrix = fitDxNDC * rh2lh * lightProjection * lispMatrix;
    m_ShadowMatrix = m_ProjMatrix * m_ViewMatrix;

    m_ClipToWorld = Invert(m_ViewProjMatrix);
	m_FrustumVS = Frustum( m_ProjMatrix );
    m_FrustumWS = Invert( m_ViewMatrix ) * m_FrustumVS;
}

