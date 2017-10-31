#include "pch.h"
#include "SMAA.h"

#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"

#include "CompiledShaders/ScreenQuadVS.h"

using namespace Graphics;

namespace SMAA 
{
    // Compute Shader
    ComputePSO Pass1HdrCS;
    ComputePSO Pass1LdrCS;
    ComputePSO ResolveWorkCS;
    ComputePSO Pass2HCS;
    ComputePSO Pass2VCS;
    ComputePSO Pass2HDebugCS;
    ComputePSO Pass2VDebugCS;

    BoolVar Enable("Graphics/AA/SMAA/Enable", false);
    BoolVar DebugDraw("Graphics/AA/FXAA/Debug", false);

    // With a properly encoded luma buffer, [0.25 = "low", 0.2 = "medium", 0.15 = "high", 0.1 = "ultra"]
    NumVar ContrastThreshold("Graphics/AA/FXAA/Contrast Threshold", 0.166f, 0.05f, 0.5f, 0.025f);
    NumVar ContrastThresholdMin("Graphics/AA/FXAA/Contrast Threshold Min", 0.0833f, 0.05f, 0.5f, 0.025f);

    // Controls how much to blur isolated pixels that have little-to-no edge length.
    NumVar SubpixelRemoval("Graphics/AA/FXAA/Subpixel Removal", 0.50f, 0.0f, 1.0f, 0.25f);

    // This is for testing the performance of computing luma on the fly rather than reusing
    // the luma buffer output of tone mapping.
    BoolVar ForceOffPreComputedLuma("Graphics/AA/FXAA/Always Recompute Log-Luma", false);
}

void SMAA::Initialize( void )
{
#if 0
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
#endif
}

void SMAA::Shutdown( void )
{
}
