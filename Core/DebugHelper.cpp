#include "pch.h"
#include "DebugHelper.h"
#include "PipelineState.h"
#include "CommandContext.h"

#include "CompiledShaders/ScreenQuadVS.h"
#include "CompiledShaders/DebugTexturePS.h"

namespace Utility
{
    GraphicsPSO DebugTexturePSO;
}

void Utility::Initialize( void )
{
    DebugTexturePSO.SetDepthStencilState( Graphics::DepthStateDisabled );
	DebugTexturePSO.SetVertexShader( MY_SHADER_ARGS( g_pScreenQuadVS ) );
	DebugTexturePSO.SetPixelShader( MY_SHADER_ARGS( g_pDebugTexturePS ) );
	DebugTexturePSO.Finalize();
}

void Utility::Shutdown( void )
{
    DebugTexturePSO.Destroy();
}

void Utility::DebugTexture( GraphicsContext& Context, D3D11_SRV_HANDLE SRV, LONG X, LONG Y, LONG W, LONG H )
{
    ScopedTimer _prof( L"Debug Texture", Context );

    D3D11_VIEWPORT Viewport = { (FLOAT)X, (FLOAT)Y, (FLOAT)W, (FLOAT)H };
    D3D11_RECT Scissor = { X, Y, W, H };
    Context.SetDynamicSampler( 0, Graphics::SamplerLinearClamp, { kBindPixel} );
    Context.SetViewportAndScissor( Viewport, Scissor );
    Context.SetPipelineState( DebugTexturePSO );
    Context.SetDynamicDescriptor( 0, SRV, { kBindPixel });
    Context.Draw(3);
    Context.SetDynamicDescriptor( 0, nullptr, { kBindPixel });
}
