#include "pch.h"
#include "Diffuse.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"

#include "CompiledShaders/ScreenQuadVS.h"
#include "CompiledShaders/DiffusePass1PS.h"
#include "CompiledShaders/DiffusePass2PS.h"

using namespace Graphics;

namespace Diffuse
{
    GraphicsPSO DiffusePass1;
    GraphicsPSO DiffusePass2;

    BoolVar Enable("Graphics/Diffuse/Enable", false);
    NumVar Extent( "Graphics/Diffuse/Extent", 0.012f, 0.0f, 0.064f, 0.001f );
}

void Diffuse::Initialize( void )
{
#define CreatePSO( ObjName, ShaderByteCode ) \
	ObjName.SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST ); \
	ObjName.SetVertexShader( MY_SHADER_ARGS( g_pScreenQuadVS ) ); \
	ObjName.SetPixelShader( MY_SHADER_ARGS( ShaderByteCode ) ); \
    ObjName.Finalize();

	CreatePSO( DiffusePass1, g_pDiffusePass1PS );
	CreatePSO( DiffusePass2, g_pDiffusePass2PS );
}

void Diffuse::Shutdown( void )
{
}

void Diffuse::Render( ComputeContext& Compute )
{
    Compute.SetDynamicDescriptor( 0, D3D11_UAV_HANDLE( nullptr ) );
    Compute.SetDynamicDescriptor( 1, D3D11_UAV_HANDLE( nullptr ) );
    Compute.SetDynamicDescriptor( 2, D3D11_SRV_HANDLE( nullptr ) );

    ScopedTimer _prof(L"Diffuse", Compute);

    ColorBuffer& Target = g_SceneColorBuffer;
    float ExtentY = Extent;
    float ExtentX = Extent * Target.GetHeight() / Target.GetWidth();
    GraphicsContext& Context = Compute.GetGraphicsContext();
    Context.SetConstants( 0, ExtentX, ExtentY, { kBindPixel } );
    Context.SetPipelineState( DiffusePass1 );
    Context.SetDynamicSampler( 0, SamplerLinearClamp, { kBindPixel } );
    Context.SetDynamicDescriptor( 0, g_SceneColorBuffer.GetSRV(), { kBindPixel } );
    Context.SetRenderTarget( g_PostEffectsBufferTyped.GetRTV() );
    Context.SetViewportAndScissor( 0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight() );
    Context.Draw(3);
    Context.SetPipelineState( DiffusePass2 );
    Context.SetRenderTarget( g_PreviousColorBuffer.GetRTV() );
    Context.SetDynamicDescriptor( 0, g_PostEffectsBufferTyped.GetSRV(), { kBindPixel } );
    Context.SetDynamicDescriptor( 1, g_SceneColorBuffer.GetSRV(), { kBindPixel } );
    Context.Draw(3);
    Context.CopyBuffer( g_SceneColorBuffer, g_PreviousColorBuffer );
}
