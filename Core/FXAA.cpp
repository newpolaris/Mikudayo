#include "pch.h"
#include "FXAA.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"

#include "CompiledShaders/FxaaVS.h"
#include "CompiledShaders/FxaaPS.h"

using namespace Graphics;

namespace FXAA
{
    BoolVar Enable("Graphics/AA/FXAA/Enable", false);
    BoolVar DebugDraw("Graphics/AA/FXAA/Debug", false);

    // With a properly encoded luma buffer, [0.25 = "low", 0.2 = "medium", 0.15 = "high", 0.1 = "ultra"]
    NumVar ContrastThreshold("Graphics/AA/FXAA/Contrast Threshold", 0.175f, 0.05f, 0.5f, 0.025f);

    // Controls how much to blur isolated pixels that have little-to-no edge length.
    NumVar SubpixelRemoval("Graphics/AA/FXAA/Subpixel Removal", 0.50f, 0.0f, 1.0f, 0.25f);

    // This is for testing the performance of computing luma on the fly rather than reusing
    // the luma buffer output of tone mapping.
    BoolVar ForceOffPreComputedLuma("Graphics/AA/FXAA/Always Recompute Log-Luma", false);

    GraphicsPSO FxaaPSO;
}

void FXAA::Initialize( void )
{
	FxaaPSO.SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	FxaaPSO.SetVertexShader( MY_SHADER_ARGS( g_pFxaaVS ) );
	FxaaPSO.SetPixelShader( MY_SHADER_ARGS( g_pFxaaPS ) );
	FxaaPSO.Finalize();
}

void FXAA::Shutdown(void)
{
    FxaaPSO.Destroy();
}

void FXAA::Render( ComputeContext& , bool bUsePreComputedLuma )
{
    if (ForceOffPreComputedLuma)
        bUsePreComputedLuma = false;

    GraphicsContext& Context = GraphicsContext::Begin(L"FXAA");
    ScopedTimer _prof(L"FXAA Processing", Context);

    Context.SetPipelineState( FxaaPSO );
    Context.SetConstants( 1.0f / g_SceneColorBuffer.GetWidth(), 1.0f / g_SceneColorBuffer.GetHeight(), { kBindPixel } );
    Context.SetDynamicSampler( 0, SamplerLinearClamp, { kBindPixel } );
    Context.SetDynamicDescriptor( 0, g_PostEffectsBuffer.GetSRV(), { kBindPixel } );
    Context.SetRenderTarget( g_SceneColorBuffer.GetRTV() );
    Context.SetViewportAndScissor( 0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight() );
    Context.Draw(3);
    Context.Finish();
}
