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
#include "FXAA.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"

#include "CompiledShaders/FXAAPass1_RGB_CS.h"
#include "CompiledShaders/FXAAPass1_Luma_CS.h"
#include "CompiledShaders/FXAAPass1_RGB2_CS.h"
#include "CompiledShaders/FXAAPass1_Luma2_CS.h"
#include "CompiledShaders/FXAAResolveWorkQueueCS.h"
#include "CompiledShaders/FXAAPass2HCS.h"
#include "CompiledShaders/FXAAPass2VCS.h"
#include "CompiledShaders/FXAAPass2H2CS.h"
#include "CompiledShaders/FXAAPass2V2CS.h"
#include "CompiledShaders/FXAAPass2HDebugCS.h"
#include "CompiledShaders/FXAAPass2VDebugCS.h"

#include "CompiledShaders/ScreenQuadVS.h"
#include "CompiledShaders/FXAAPS.h"

using namespace Graphics;

namespace FXAA
{
    // Compute Shader
    ComputePSO Pass1HdrCS;
    ComputePSO Pass1LdrCS;
    ComputePSO ResolveWorkCS;
    ComputePSO Pass2HCS;
    ComputePSO Pass2VCS;
    ComputePSO Pass2HDebugCS;
    ComputePSO Pass2VDebugCS;
    IndirectArgsBuffer IndirectParameters;

    // Pixel Shader
    GraphicsPSO FXAAPS;

    BoolVar Enable("Graphics/AA/FXAA/Enable", false);
    BoolVar DebugDraw("Graphics/AA/FXAA/Debug", false);
    // TODO: PostEffects modification is needed.
    BoolVar ForceCS("Graphics/AA/FXAA/ForceCS", true);

    // With a properly encoded luma buffer, [0.25 = "low", 0.2 = "medium", 0.15 = "high", 0.1 = "ultra"]
    NumVar ContrastThreshold("Graphics/AA/FXAA/Contrast Threshold", 0.166f, 0.05f, 0.5f, 0.025f);
    NumVar ContrastThresholdMin("Graphics/AA/FXAA/Contrast Threshold Min", 0.0833f, 0.05f, 0.5f, 0.025f);

    // Controls how much to blur isolated pixels that have little-to-no edge length.
    NumVar SubpixelRemoval("Graphics/AA/FXAA/Subpixel Removal", 0.50f, 0.0f, 1.0f, 0.25f);

    // This is for testing the performance of computing luma on the fly rather than reusing
    // the luma buffer output of tone mapping.
    BoolVar ForceOffPreComputedLuma("Graphics/AA/FXAA/Always Recompute Log-Luma", false);
}

void FXAA::Initialize( void )
{
#define CreatePSO( ObjName, ShaderByteCode ) \
    ObjName.SetComputeShader( MY_SHADER_ARGS(ShaderByteCode) ); \
    ObjName.Finalize();

    CreatePSO(ResolveWorkCS, g_pFXAAResolveWorkQueueCS);
    if (g_bTypedUAVLoadSupport_R11G11B10_FLOAT)
    {
        CreatePSO(Pass1LdrCS, g_pFXAAPass1_RGB2_CS);    // Use RGB and recompute log-luma; pre-computed luma is unavailable
        CreatePSO(Pass1HdrCS, g_pFXAAPass1_Luma2_CS);   // Use pre-computed luma
        CreatePSO(Pass2HCS, g_pFXAAPass2H2CS);
        CreatePSO(Pass2VCS, g_pFXAAPass2V2CS);
    }
    else
    {
        CreatePSO(Pass1LdrCS, g_pFXAAPass1_RGB_CS);     // Use RGB and recompute log-luma; pre-computed luma is unavailable
        CreatePSO(Pass1HdrCS, g_pFXAAPass1_Luma_CS);    // Use pre-computed luma
        CreatePSO(Pass2HCS, g_pFXAAPass2HCS);
        CreatePSO(Pass2VCS, g_pFXAAPass2VCS);
    }
    CreatePSO(Pass2HDebugCS, g_pFXAAPass2HDebugCS);
    CreatePSO(Pass2VDebugCS, g_pFXAAPass2VDebugCS);
#undef CreatePSO

    __declspec(align(16)) const uint32_t initArgs[6] = { 0, 1, 1, 0, 1, 1 };
    IndirectParameters.Create(L"FXAA Indirect Parameters", 2, sizeof(uint32_t) * _countof(initArgs), initArgs);

	FXAAPS.SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	FXAAPS.SetVertexShader( MY_SHADER_ARGS( g_pScreenQuadVS ) );
	FXAAPS.SetPixelShader( MY_SHADER_ARGS( g_pFXAAPS ) );
	FXAAPS.Finalize();
}

void FXAA::Shutdown(void)
{
    IndirectParameters.Destroy();
}

void FXAA::Render( ComputeContext& Context, bool bUsePreComputedLuma )
{
    if (g_bTypedUAVLoadSupport_R11G11B10_FLOAT || ForceCS)
        RenderCS( Context, bUsePreComputedLuma );
    else
        RenderPS( Context, bUsePreComputedLuma );
}

void FXAA::RenderCS( ComputeContext& Context, bool bUsePreComputedLuma )
{
    ScopedTimer _prof(L"FXAA", Context);

    if (ForceOffPreComputedLuma)
        bUsePreComputedLuma = false;

    ColorBuffer& Target = g_bTypedUAVLoadSupport_R11G11B10_FLOAT ? g_SceneColorBuffer : g_PostEffectsBuffer;

	float Constants[] = { 1.0f / Target.GetWidth(), 1.0f / Target.GetHeight(), (float)ContrastThreshold, (float)ContrastThresholdMin, (float)SubpixelRemoval };
    Context.SetConstants( 0, _countof(Constants), Constants);

    {
        ScopedTimer _prof2(L"Pass 1", Context);

        // Begin by analysing the luminance buffer and setting aside high-contrast pixels in
        // work queues to be processed later.  There are horizontal edge and vertical edge work
        // queues so that the shader logic is simpler for each type of edge.
        D3D11_UAV_HANDLE Pass1UAVs[] =
        {
            g_FXAAWorkQueueH.GetUAV(),
            g_FXAAColorQueueH.GetUAV(),
            g_FXAAWorkQueueV.GetUAV(),
            g_FXAAColorQueueV.GetUAV(),
            g_LumaBuffer.GetUAV()
        };

        std::vector<UINT> Pass1UAVsInit( _countof(Pass1UAVs) );

        D3D11_SRV_HANDLE Pass1SRVs[] =
        {
            Target.GetSRV(),
            g_LumaBuffer.GetSRV()
        };

        Context.SetDynamicSampler( 0, SamplerLinearClamp );
        if (bUsePreComputedLuma)
        {
            Context.SetPipelineState(Pass1HdrCS);
            Context.SetDynamicDescriptors(0, _countof(Pass1UAVs) - 1, Pass1UAVs, Pass1UAVsInit.data());
            Context.SetDynamicDescriptors(0, _countof(Pass1SRVs), Pass1SRVs);
        }
        else
        {
            Context.SetPipelineState(Pass1LdrCS);
            Context.SetDynamicDescriptors(0, _countof(Pass1UAVs), Pass1UAVs, Pass1UAVsInit.data());
            Context.SetDynamicDescriptors(0, _countof(Pass1SRVs) - 1, Pass1SRVs);
        }

        Context.Dispatch2D(Target.GetWidth(), Target.GetHeight());
    }

    {
        ScopedTimer _prof2(L"Pass 2", Context);

        g_FXAAWorkQueueH.FillCounter( Context );
        g_FXAAWorkQueueV.FillCounter( Context );

        // The next phase involves converting the work queues to DispatchIndirect parameters.
        // The queues are also padded out to 64 elements to simplify the final consume logic.
        Context.SetPipelineState(ResolveWorkCS);
        // Context.TransitionResource(IndirectParameters, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        Context.SetConstants(0, 1.0f / Target.GetWidth(), 1.0f / Target.GetHeight());

        Context.SetDynamicDescriptor(0, IndirectParameters.GetUAV());
        Context.SetDynamicDescriptor(1, g_FXAAWorkQueueH.GetUAV());
        Context.SetDynamicDescriptor(2, g_FXAAWorkQueueV.GetUAV());
        Context.SetDynamicDescriptor(0, g_FXAAWorkQueueH.GetCounterSRV());
        Context.SetDynamicDescriptor(1, g_FXAAWorkQueueV.GetCounterSRV());

        Context.Dispatch(1, 1, 1);

        Context.SetDynamicDescriptor(0, Target.GetUAV());
        Context.SetDynamicDescriptor(1, D3D11_UAV_HANDLE(nullptr));
        Context.SetDynamicDescriptor(2, D3D11_UAV_HANDLE(nullptr));
        Context.SetDynamicDescriptor(3, D3D11_UAV_HANDLE(nullptr));
        Context.SetDynamicDescriptor(4, D3D11_UAV_HANDLE(nullptr));

        Context.SetDynamicDescriptor(0, g_LumaBuffer.GetSRV());
        Context.SetDynamicDescriptor(1, g_FXAAWorkQueueH.GetSRV());
        Context.SetDynamicDescriptor(2, g_FXAAColorQueueH.GetSRV());

        // The final phase involves processing pixels on the work queues and writing them
        // back into the color buffer.  Because the two source pixels required for linearly
        // blending are held in the work queue, this does not require also sampling from
        // the target color buffer (i.e. no read/modify/write, just write.)

        Context.SetDynamicSampler( 0, SamplerLinearClamp );
        Context.SetPipelineState(DebugDraw ? Pass2HDebugCS : Pass2HCS);
        Context.DispatchIndirect(IndirectParameters, 0);

        Context.SetDynamicDescriptor(1, g_FXAAWorkQueueV.GetSRV());
        Context.SetDynamicDescriptor(2, g_FXAAColorQueueV.GetSRV());

        Context.SetPipelineState(DebugDraw ? Pass2VDebugCS : Pass2VCS);
        Context.DispatchIndirect(IndirectParameters, 12);
    }
}

void FXAA::RenderPS( ComputeContext& Compute, bool bUsePreComputedLuma )
{
    ASSERT( bUsePreComputedLuma, "Not supported yet" );

    Compute.SetDynamicDescriptor( 0, D3D11_UAV_HANDLE( nullptr ) );
    Compute.SetDynamicDescriptor( 1, D3D11_UAV_HANDLE( nullptr ) );
    Compute.SetDynamicDescriptor( 2, D3D11_SRV_HANDLE( nullptr ) );

    ScopedTimer _prof(L"FXAA", Compute);

    ColorBuffer& Target = g_SceneColorBuffer;

    GraphicsContext& Context = Compute.GetGraphicsContext();
    Context.SetPipelineState( FXAAPS );
	float Constants[] = { 1.0f / Target.GetWidth(), 1.0f / Target.GetHeight(), (float)ContrastThreshold, (float)ContrastThresholdMin, (float)SubpixelRemoval };
    Context.SetConstants( 0, _countof(Constants), Constants, { kBindPixel } );
    Context.SetDynamicSampler( 0, SamplerLinearClamp, { kBindPixel } );
    Context.SetDynamicDescriptor( 0, g_PostEffectsBufferTyped.GetSRV(), { kBindPixel } );
    Context.SetDynamicDescriptor( 1, g_LumaBuffer.GetSRV(), { kBindPixel } );
    Context.SetRenderTarget( g_SceneColorBuffer.GetRTV() );
    Context.SetViewportAndScissor( 0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight() );
    Context.Draw(3);
}
