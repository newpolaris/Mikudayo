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

#include "PostEffects.h"
#include "GameCore.h"
#include "CommandContext.h"
#include "PipelineState.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "MotionBlur.h"
#include "DepthOfField.h"
#include "TextureManager.h"
#include "FXAA.h"

#include "CompiledShaders/ApplyBloomCS.h"
#include "CompiledShaders/ApplyBloom2CS.h"
#include "CompiledShaders/DebugLuminanceLdrCS.h"
#include "CompiledShaders/DebugLuminanceLdr2CS.h"
#include "CompiledShaders/CopyBackPostBufferCS.h"

using namespace Graphics;

namespace SSAO
{
    extern BoolVar DebugDraw;
}

namespace FXAA
{
    extern BoolVar DebugDraw;
}

namespace DepthOfField
{
    extern BoolVar Enable;
    extern EnumVar DebugMode;
}

namespace PostEffects
{
    const float kInitialMinLog = -12.0f;
    const float kInitialMaxLog = 4.0f;

    BoolVar EnableHDR("Graphics/HDR/Enable", false);
    BoolVar EnableAdaptation("Graphics/HDR/Adaptive Exposure", true);
    ExpVar MinExposure("Graphics/HDR/Min Exposure", 1.0f / 64.0f, -8.0f, 0.0f, 0.25f);
    ExpVar MaxExposure("Graphics/HDR/Max Exposure", 64.0f, 0.0f, 8.0f, 0.25f);
    NumVar TargetLuminance("Graphics/HDR/Key", 0.08f, 0.01f, 0.99f, 0.01f);
    NumVar AdaptationRate("Graphics/HDR/Adaptation Rate", 0.05f, 0.01f, 1.0f, 0.01f);
    ExpVar Exposure("Graphics/HDR/Exposure", 2.0f, -8.0f, 8.0f, 0.25f);
    BoolVar DrawHistogram("Graphics/HDR/Draw Histogram", false);

    BoolVar BloomEnable("Graphics/Bloom/Enable", true);
    NumVar BloomThreshold("Graphics/Bloom/Threshold", 4.0f, 0.0f, 8.0f, 0.1f);		// The threshold luminance above which a pixel will start to bloom
    NumVar BloomStrength("Graphics/Bloom/Strength", 0.1f, 0.0f, 2.0f, 0.05f);		// A modulator controlling how much bloom is added back into the image
    NumVar BloomUpsampleFactor("Graphics/Bloom/Scatter", 0.65f, 0.0f, 1.0f, 0.05f);	// Controls the "focus" of the blur.  High values spread out more causing a haze.
    BoolVar HighQualityBloom("Graphics/Bloom/High Quality", true);					// High quality blurs 5 octaves of bloom; low quality only blurs 3.

    ComputePSO ToneMapCS;
    ComputePSO ToneMapHDRCS;
    ComputePSO ApplyBloomCS;
    ComputePSO DebugLuminanceHdrCS;
    ComputePSO DebugLuminanceLdrCS;
    ComputePSO GenerateHistogramCS;
    ComputePSO DrawHistogramCS;
    ComputePSO AdaptExposureCS;
    ComputePSO DownsampleBloom2CS;
    ComputePSO DownsampleBloom4CS;
    ComputePSO UpsampleAndBlurCS;
    ComputePSO BlurCS;
    ComputePSO BloomExtractAndDownsampleHdrCS;
    ComputePSO BloomExtractAndDownsampleLdrCS;
    ComputePSO ExtractLumaCS;
    ComputePSO AverageLumaCS;
    ComputePSO CopyBackPostBufferCS;


    void UpdateExposure(ComputeContext&);
    void BlurBuffer(ComputeContext&, ColorBuffer buffer[2], const ColorBuffer& lowerResBuf, float upsampleBlendFactor );
    void GenerateBloom(ComputeContext&);
    void ExtractLuma(ComputeContext&);
    void ProcessHDR(ComputeContext&);
    void ProcessLDR(CommandContext&);
}

void PostEffects::Initialize( void )
{
    FXAA::Initialize();
    MotionBlur::Initialize();
    DepthOfField::Initialize();

#define CreatePSO( ObjName, ShaderByteCode ) \
	ObjName.SetComputeShader( MY_SHADER_ARGS( ShaderByteCode) ); \
    ObjName.Finalize();

    if (g_bTypedUAVLoadSupport_R11G11B10_FLOAT)
    {
        CreatePSO( ApplyBloomCS, g_pApplyBloom2CS );
        CreatePSO( DebugLuminanceLdrCS, g_pDebugLuminanceLdr2CS );
    }
    else
    {
        CreatePSO( ApplyBloomCS, g_pApplyBloomCS );
        CreatePSO( DebugLuminanceLdrCS, g_pDebugLuminanceLdrCS );
    }

    CreatePSO( CopyBackPostBufferCS, g_pCopyBackPostBufferCS );
}

void PostEffects::Shutdown( void )
{
    ToneMapCS.Destroy();
    ToneMapHDRCS.Destroy();
    ApplyBloomCS.Destroy();
    DebugLuminanceHdrCS.Destroy();
    DebugLuminanceLdrCS.Destroy();
    GenerateHistogramCS.Destroy();
    DrawHistogramCS.Destroy();
    AdaptExposureCS.Destroy();
    DownsampleBloom2CS.Destroy();
    DownsampleBloom4CS.Destroy();
    UpsampleAndBlurCS.Destroy();
    BlurCS.Destroy();
    BloomExtractAndDownsampleHdrCS.Destroy();
    BloomExtractAndDownsampleLdrCS.Destroy();
    ExtractLumaCS.Destroy();
    AverageLumaCS.Destroy();
    CopyBackPostBufferCS.Destroy();

    FXAA::Shutdown();
    MotionBlur::Shutdown();
    DepthOfField::Shutdown();
}

//--------------------------------------------------------------------------------------
// Bloom effect in CS path
//--------------------------------------------------------------------------------------
void PostEffects::GenerateBloom( ComputeContext& Context )
{
}

void PostEffects::ProcessHDR( ComputeContext& Context )
{
}

void PostEffects::ProcessLDR( CommandContext& BaseContext )
{
    ScopedTimer _prof(L"SDR Processing", BaseContext);

    ComputeContext& Context = BaseContext.GetComputeContext();

    bool bGenerateBloom = BloomEnable && !SSAO::DebugDraw;
    if (bGenerateBloom)
        GenerateBloom(Context);

    if (bGenerateBloom || FXAA::DebugDraw || SSAO::DebugDraw || !g_bTypedUAVLoadSupport_R11G11B10_FLOAT)
    {
        // Set constants
        Context.SetConstants( 0, 1.0f / g_SceneColorBuffer.GetWidth(), 1.0f / g_SceneColorBuffer.GetHeight(),
            (float)BloomStrength);

        // Separate out SDR result from its perceived luminance
        if (g_bTypedUAVLoadSupport_R11G11B10_FLOAT)
            Context.SetDynamicDescriptor(0, g_SceneColorBuffer.GetUAV());
        else
        {
            Context.SetDynamicDescriptor(0, g_PostEffectsBuffer.GetUAV());
            Context.SetDynamicDescriptor(2, g_SceneColorBuffer.GetSRV());
        }
        Context.SetDynamicDescriptor(1, g_LumaBuffer.GetUAV());

        // Read in original SDR value and blurred bloom buffer
        Context.SetDynamicDescriptor(0, bGenerateBloom ? g_aBloomUAV1[1].GetSRV() : TextureManager::GetBlackTex2D().GetSRV());

        Context.SetDynamicSampler( 0, SamplerLinearClamp );
        Context.SetPipelineState(FXAA::DebugDraw ? DebugLuminanceLdrCS : ApplyBloomCS);
        Context.Dispatch2D(g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());
    }
}

void PostEffects::CopyBackPostBuffer( ComputeContext& Context )
{
	ScopedTimer _prof(L"Copy Post back to Scene", Context);
    Context.SetPipelineState(CopyBackPostBufferCS);
    Context.SetDynamicSampler(0, SamplerLinearClamp);
    Context.SetDynamicDescriptor(2, D3D11_SRV_HANDLE(nullptr));
    Context.SetDynamicDescriptor(0, g_SceneColorBuffer.GetUAV());
    Context.SetDynamicDescriptor(0, g_PostEffectsBuffer.GetSRV());
    Context.Dispatch2D(g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());
}

void PostEffects::Render( void )
{
    ComputeContext& Context = ComputeContext::Begin(L"Post Effects");

    if (EnableHDR && !SSAO::DebugDraw && !(DepthOfField::Enable && DepthOfField::DebugMode >= 3))
        ProcessHDR(Context);
    else
        ProcessLDR(Context);

    bool bGeneratedLumaBuffer = EnableHDR || FXAA::DebugDraw || BloomEnable;
    if (FXAA::Enable)
        FXAA::Render(Context, bGeneratedLumaBuffer);

    // In the case where we've been doing post processing in a separate buffer, we need to copy it
    // back to the original buffer.  It is possible to skip this step if the next shader knows to
    // do the manual format decode from UINT, but there are several code paths that need to be
    // changed, and some of them rely on texture filtering, which won't work with UINT.  Since this
    // is only to support legacy hardware and a single buffer copy isn't that big of a deal, this
    // is the most economical solution.
    if (!g_bTypedUAVLoadSupport_R11G11B10_FLOAT)
        CopyBackPostBuffer(Context);

    Context.Finish();
}
