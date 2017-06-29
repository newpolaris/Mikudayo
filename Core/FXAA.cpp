#include "pch.h"
#include "FXAA.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"

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
}

void FXAA::Initialize( void )
{
}

void FXAA::Shutdown(void)
{
}

void FXAA::Render( ComputeContext& Context, bool bUsePreComputedLuma )
{
    if (ForceOffPreComputedLuma)
        bUsePreComputedLuma = false;

    ColorBuffer& Target = g_bTypedUAVLoadSupport_R11G11B10_FLOAT ? g_SceneColorBuffer : g_PostEffectsBuffer;
}
