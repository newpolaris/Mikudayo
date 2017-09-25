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
#include "FXAA.h"
#include "TextureManager.h"

#include "CompiledShaders/ToneMapCS.h"
#include "CompiledShaders/ToneMap2CS.h"
#include "CompiledShaders/ToneMapHDRCS.h"
#include "CompiledShaders/ToneMapHDR2CS.h"
#include "CompiledShaders/ApplyBloomCS.h"
#include "CompiledShaders/ApplyBloom2CS.h"
#include "CompiledShaders/ApplyBloom3CS.h" // Support pixel shader Fxaa
#include "CompiledShaders/DebugLuminanceHdrCS.h"
#include "CompiledShaders/DebugLuminanceHdr2CS.h"
#include "CompiledShaders/DebugLuminanceLdrCS.h"
#include "CompiledShaders/DebugLuminanceLdr2CS.h"
#include "CompiledShaders/GenerateHistogramCS.h"
// #include "CompiledShaders/DebugDrawHistogramCS.h"
#include "CompiledShaders/AdaptExposureCS.h"
// #include "CompiledShaders/DownsampleBloomCS.h"
// #include "CompiledShaders/DownsampleBloomAllCS.h"
// #include "CompiledShaders/UpsampleAndBlurCS.h"
// #include "CompiledShaders/BlurCS.h"
// #include "CompiledShaders/BloomExtractAndDownsampleHdrCS.h"
// #include "CompiledShaders/BloomExtractAndDownsampleLdrCS.h"
#include "CompiledShaders/ExtractLumaCS.h"
// #include "CompiledShaders/AverageLumaCS.h"
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

    BoolVar EnableHDR("Graphics/HDR/Enable", true);
    BoolVar EnableAdaptation("Graphics/HDR/Adaptive Exposure", false);
    ExpVar MinExposure("Graphics/HDR/Min Exposure", 1.0f / 64.0f, -8.0f, 0.0f, 0.25f);
    ExpVar MaxExposure("Graphics/HDR/Max Exposure", 64.0f, 0.0f, 8.0f, 0.25f);
    NumVar TargetLuminance("Graphics/HDR/Key", 0.08f, 0.01f, 0.99f, 0.01f);
    NumVar AdaptationRate("Graphics/HDR/Adaptation Rate", 0.05f, 0.01f, 1.0f, 0.01f);
    ExpVar Exposure("Graphics/HDR/Exposure", 2.0f, -8.0f, 8.0f, 0.25f);
    BoolVar DrawHistogram("Graphics/HDR/Draw Histogram", false);
    BoolVar BloomEnable("Graphics/Bloom/Enable", false);
    NumVar BloomThreshold("Graphics/Bloom/Threshold", 4.0f, 0.0f, 8.0f, 0.1f);		// The threshold luminance above which a pixel will start to bloom
    NumVar BloomStrength("Graphics/Bloom/Strength", 0.1f, 0.0f, 2.0f, 0.05f);		// A modulator controlling how much bloom is added back into the image
    NumVar BloomUpsampleFactor("Graphics/Bloom/Scatter", 0.65f, 0.0f, 1.0f, 0.05f);	// Controls the "focus" of the blur.  High values spread out more causing a haze.
    BoolVar HighQualityBloom("Graphics/Bloom/High Quality", true);					// High quality blurs 5 octaves of bloom; low quality only blurs 3.

    ComputePSO ToneMapCS;
    ComputePSO ToneMapHDRCS;
    ComputePSO ApplyBloomCS;
    ComputePSO ApplyBloomFxaaPS_CS;
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

    StructuredBuffer g_Exposure;

    void UpdateExposure(ComputeContext& Context);
    void BlurBuffer(ComputeContext&, ColorBuffer buffer[2], const ColorBuffer& lowerResBuf, float upsampleBlendFactor );
    void GenerateBloom(ComputeContext&);
    void ExtractLuma(ComputeContext& Context);
    void ProcessHDR(ComputeContext& Context);
    void ProcessLDR(CommandContext& Context);
}

void PostEffects::Initialize( void )
{

#define CreatePSO( ObjName, ShaderByteCode ) \
	ObjName.SetComputeShader( MY_SHADER_ARGS( ShaderByteCode) ); \
    ObjName.Finalize();

    if (g_bTypedUAVLoadSupport_R11G11B10_FLOAT)
    {
        CreatePSO(ToneMapCS, g_pToneMap2CS);
        CreatePSO(ToneMapHDRCS, g_pToneMapHDR2CS);
        CreatePSO(ApplyBloomCS, g_pApplyBloom2CS);
        CreatePSO(DebugLuminanceHdrCS, g_pDebugLuminanceHdr2CS);
        CreatePSO(DebugLuminanceLdrCS, g_pDebugLuminanceLdr2CS);
    }
    else
    {
        CreatePSO(ToneMapCS, g_pToneMapCS);
        CreatePSO(ToneMapHDRCS, g_pToneMapHDRCS);
        CreatePSO(ApplyBloomCS, g_pApplyBloomCS);
        CreatePSO(DebugLuminanceHdrCS, g_pDebugLuminanceHdrCS);
        CreatePSO(DebugLuminanceLdrCS, g_pDebugLuminanceLdrCS);
    }
	CreatePSO( ApplyBloomFxaaPS_CS, g_pApplyBloom3CS );
    CreatePSO( GenerateHistogramCS, g_pGenerateHistogramCS );
    // CreatePSO( DrawHistogramCS, g_pDebugDrawHistogramCS );
    CreatePSO( AdaptExposureCS, g_pAdaptExposureCS );
    // CreatePSO( DownsampleBloom2CS, g_pDownsampleBloomCS );
    // CreatePSO( DownsampleBloom4CS, g_pDownsampleBloomAllCS );
    // CreatePSO( UpsampleAndBlurCS, g_pUpsampleAndBlurCS );
    // CreatePSO( BlurCS, g_pBlurCS );
    // CreatePSO( BloomExtractAndDownsampleHdrCS, g_pBloomExtractAndDownsampleHdrCS );
    // CreatePSO( BloomExtractAndDownsampleLdrCS, g_pBloomExtractAndDownsampleLdrCS );
    CreatePSO( ExtractLumaCS, g_pExtractLumaCS );
    // CreatePSO( AverageLumaCS, g_pAverageLumaCS );
    CreatePSO( CopyBackPostBufferCS, g_pCopyBackPostBufferCS );


#undef CreatePSO

    __declspec(align(16)) float initExposure[] =
    {
        Exposure, 1.0f / Exposure, Exposure, 0.0f,
        kInitialMinLog, kInitialMaxLog, kInitialMaxLog - kInitialMinLog, 1.0f / (kInitialMaxLog - kInitialMinLog)
    };
    g_Exposure.Create(L"Exposure", 8, 4, initExposure);

    FXAA::Initialize();
    MotionBlur::Initialize();
    DepthOfField::Initialize();
}

void PostEffects::Shutdown( void )
{
    g_Exposure.Destroy();

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

void PostEffects::ExtractLuma(ComputeContext& Context)
{
    ScopedTimer _prof(L"Extract Luma", Context);

    Context.TransitionResource(g_LumaLR, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Context.TransitionResource(g_Exposure, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Context.SetConstants(0, 1.0f / g_LumaLR.GetWidth(), 1.0f / g_LumaLR.GetHeight());
    Context.SetDynamicDescriptor(0, g_LumaLR.GetUAV());
    Context.SetDynamicDescriptor(0, g_SceneColorBuffer.GetSRV());
    Context.SetDynamicDescriptor(1, g_Exposure.GetSRV());
    Context.SetDynamicSampler( 0, SamplerLinearClamp );
    Context.SetDynamicSampler( 1, SamplerLinearBorder );
    Context.SetPipelineState(ExtractLumaCS);
    Context.Dispatch2D(g_LumaLR.GetWidth(), g_LumaLR.GetHeight());
}

void PostEffects::UpdateExposure(ComputeContext& Context)
{
    ScopedTimer _prof(L"Update Exposure", Context);

    if (!EnableAdaptation)
    {
        __declspec(align(16)) float initExposure[] =
        {
            Exposure, 1.0f / Exposure, Exposure, 0.0f,
            kInitialMinLog, kInitialMaxLog, kInitialMaxLog - kInitialMinLog, 1.0f / (kInitialMaxLog - kInitialMinLog)
        };
        Context.WriteBuffer(g_Exposure, 0, initExposure, sizeof(initExposure));
        Context.TransitionResource(g_Exposure, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        return;
    }

    // Generate an HDR histogram
    Context.TransitionResource(g_Histogram, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
    Context.ClearUAV(g_Histogram);
    Context.TransitionResource(g_LumaLR, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Context.SetDynamicDescriptor(0, g_Histogram.GetUAV() );
    Context.SetDynamicDescriptor(0, g_LumaLR.GetSRV() );
    Context.SetDynamicSampler( 0, SamplerLinearClamp );
    Context.SetDynamicSampler( 1, SamplerLinearBorder );
    Context.SetPipelineState(GenerateHistogramCS);
    Context.Dispatch2D(g_LumaLR.GetWidth(), g_LumaLR.GetHeight(), 16, 384);

    __declspec(align(16)) struct
    {
        float TargetLuminance;
        float AdaptationRate;
        float MinExposure;
        float MaxExposure;
        uint32_t PixelCount;
    } constants =
    {
        TargetLuminance, AdaptationRate, MinExposure, MaxExposure,
        g_LumaLR.GetWidth() * g_LumaLR.GetHeight()
    };
    Context.TransitionResource(g_Histogram, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    Context.TransitionResource(g_Exposure, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Context.SetDynamicDescriptor(0, g_Exposure.GetUAV());
    Context.SetDynamicDescriptor(0, g_Histogram.GetSRV());
    Context.SetDynamicConstantBufferView(1, sizeof(constants), &constants);
    Context.SetDynamicSampler( 0, SamplerLinearClamp );
    Context.SetDynamicSampler( 1, SamplerLinearBorder );
    Context.SetPipelineState(AdaptExposureCS);
    Context.Dispatch();
    Context.TransitionResource(g_Exposure, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void PostEffects::ProcessHDR( ComputeContext& Context )
{
    ScopedTimer _prof(L"HDR Tone Mapping", Context);

    if (BloomEnable)
    {
        GenerateBloom(Context);
        Context.TransitionResource(g_aBloomUAV1[1], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }
    else if (EnableAdaptation)
        ExtractLuma(Context);

    if (g_bTypedUAVLoadSupport_R11G11B10_FLOAT)
        Context.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    else
        Context.TransitionResource(g_PostEffectsBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    Context.TransitionResource(g_LumaBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Context.TransitionResource(g_Exposure, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    Context.SetDynamicSampler( 0, SamplerLinearClamp );
    Context.SetPipelineState(FXAA::DebugDraw ? DebugLuminanceHdrCS : (g_bEnableHDROutput ? ToneMapHDRCS : ToneMapCS));

    // Set constants
    Context.SetConstants(0, 1.0f / g_SceneColorBuffer.GetWidth(), 1.0f / g_SceneColorBuffer.GetHeight(),
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

    // Read in original HDR value and blurred bloom buffer
    Context.SetDynamicDescriptor(0, g_Exposure.GetSRV());
    Context.SetDynamicDescriptor(1, BloomEnable ? g_aBloomUAV1[1].GetSRV() : TextureManager::GetBlackTex2D().GetSRV());
    
    Context.Dispatch2D(g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());

    // Do this last so that the bright pass uses the same exposure as tone mapping
    UpdateExposure(Context);
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
		if (g_bTypedUAVLoadSupport_R11G11B10_FLOAT)
            Context.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        else
            Context.TransitionResource(g_PostEffectsBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        Context.TransitionResource(g_aBloomUAV1[1], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        Context.TransitionResource(g_LumaBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

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

        Context.TransitionResource(g_LumaBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
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

    if (!g_bTypedUAVLoadSupport_R11G11B10_FLOAT)
        CopyBackPostBuffer(Context);

    Context.Finish();
}
