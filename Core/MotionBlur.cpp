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
#include "MotionBlur.h"
#include "Camera.h"
#include "BufferManager.h"
#include "GraphicsCore.h"
#include "CommandContext.h"
#include "PostEffects.h"
#include "SystemTime.h"
#include "DebugHelper.h"

#include "CompiledShaders/ScreenQuadVS.h"
#include "CompiledShaders/MotionVector.h"
#include "CompiledShaders/MotionBlur.h"

using namespace Graphics;
using namespace Math;
using namespace TemporalAA;

namespace MotionBlur
{
	BoolVar Enable("Graphics/Motion Blur/Enable", false);

	ComputePSO s_CameraMotionBlurPrePassCS[2];
	ComputePSO s_MotionBlurPrePassCS;
	ComputePSO s_MotionBlurFinalPassCS;
	GraphicsPSO s_MotionBlurFinalPassPS;
	ComputePSO s_CameraVelocityCS[2];
    // Test
    ComputePSO s_CameraVelocityPSO;
    ComputePSO s_CameraBlurPSO;
}

void MotionBlur::Initialize( void )
{
    s_CameraVelocityPSO.SetComputeShader(MY_SHADER_ARGS(g_pMotionVector));
    s_CameraVelocityPSO.Finalize();

    s_CameraBlurPSO.SetComputeShader(MY_SHADER_ARGS(g_pMotionBlur));
    s_CameraBlurPSO.Finalize();
}

void MotionBlur::Shutdown( void )
{
}

// Linear Z ends up being faster since we haven't officially decompressed the depth buffer.  You
// would think that it might be slower to use linear Z because we have to convert it back to
// hyperbolic Z for the reprojection.  Nevertheless, the reduced bandwidth and decompress eliminate
// make Linear Z the better choice.  (The choice also lets you evict the depth buffer from ESRAM.)

void MotionBlur::GenerateCameraVelocityBuffer( CommandContext& BaseContext, const Camera& camera, bool UseLinearZ )
{
    GenerateCameraVelocityBuffer(BaseContext, camera.GetReprojectionMatrix(), camera.GetNearClip(), camera.GetFarClip(), UseLinearZ);
}

void MotionBlur::GenerateCameraVelocityBuffer( CommandContext& BaseContext, const Matrix4& reprojectionMatrix, float nearClip, float farClip, bool UseLinearZ )
{
    ScopedTimer _prof(L"Generate Camera Velocity", BaseContext);

    uint32_t Width = g_SceneColorBuffer.GetWidth();
    uint32_t Height = g_SceneColorBuffer.GetHeight();

    struct ScreenToViewParams
    {
        Matrix4 Reprojection;
        Vector4 ScreenDimensions;
    } csScreenToView;
    csScreenToView.Reprojection = reprojectionMatrix;
    csScreenToView.ScreenDimensions = Vector4( float(Width), float(Height), 0, 0 );
    ComputeContext& Context = BaseContext.GetComputeContext();
    Context.SetDynamicConstantBufferView( 0, sizeof(csScreenToView), &csScreenToView );
    Context.SetPipelineState( s_CameraVelocityPSO );
    Context.SetDynamicDescriptor( 0, g_SceneDepthBuffer.GetDepthSRV() );
    Context.SetDynamicDescriptor( 0, g_VelocityBuffer.GetUAV() );
    Context.Dispatch2D( Width, Height );
    Context.SetDynamicDescriptor( 0, UAV_NULL );
}

void MotionBlur::RenderObjectBlur( CommandContext& BaseContext, ColorBuffer& velocityBuffer )
{
    ScopedTimer _prof(L"MotionBlur", BaseContext);

    uint32_t Width = g_SceneColorBuffer.GetWidth();
    uint32_t Height = g_SceneColorBuffer.GetHeight();

    Vector4 screenDimensions = Vector4( float(g_SceneColorBuffer.GetWidth()), float(g_SceneColorBuffer.GetHeight()), 0, 0 );
    ComputeContext& Context = BaseContext.GetComputeContext();
    Context.SetDynamicConstantBufferView( 0, sizeof(screenDimensions), &screenDimensions );
    Context.SetPipelineState( s_CameraBlurPSO );
    Context.SetDynamicDescriptor( 0, g_VelocityBuffer.GetSRV() );
    Context.SetDynamicDescriptor( 1, g_SceneColorBuffer.GetSRV() );
    Context.SetDynamicDescriptor( 0, g_PostEffectsBufferTyped.GetUAV() );
    Context.Dispatch2D( Width, Height );
    Context.SetDynamicDescriptor( 0, UAV_NULL );
    BaseContext.CopyBuffer( g_SceneColorBuffer, g_PostEffectsBufferTyped );
}
