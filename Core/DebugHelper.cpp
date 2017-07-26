#include "pch.h"
#include "DebugHelper.h"
#include "PipelineState.h"
#include "CommandContext.h"
#include "InputLayout.h"
#include "GpuBuffer.h"

#include "CompiledShaders/ScreenQuadVS.h"
#include "CompiledShaders/DebugTexturePS.h"
#include "CompiledShaders/PrimitiveVS.h"
#include "CompiledShaders/PrimitivePS.h"

namespace Utility
{
    GraphicsPSO DebugTexturePSO;
    GraphicsPSO RenderCubePSO;
    GraphicsPSO RenderCubeWirePSO;
}

void Utility::Initialize( void )
{
    DebugTexturePSO.SetDepthStencilState( Graphics::DepthStateDisabled );
	DebugTexturePSO.SetVertexShader( MY_SHADER_ARGS( g_pScreenQuadVS ) );
	DebugTexturePSO.SetPixelShader( MY_SHADER_ARGS( g_pDebugTexturePS ) );
	DebugTexturePSO.Finalize();

	InputDesc CubeInputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "GARBAGE", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
    RenderCubePSO.SetInputLayout( _countof(CubeInputLayout), CubeInputLayout );
	RenderCubePSO.SetVertexShader( MY_SHADER_ARGS( g_pPrimitiveVS ) );
	RenderCubePSO.SetPixelShader( MY_SHADER_ARGS( g_pPrimitivePS ) );
    RenderCubePSO.SetBlendState( Graphics::BlendTraditional );
    RenderCubePSO.SetRasterizerState( Graphics::RasterizerTwoSided );
	RenderCubePSO.Finalize();

    RenderCubeWirePSO = RenderCubePSO;
    RenderCubeWirePSO.SetRasterizerState( Graphics::RasterizerWireframe );
    RenderCubeWirePSO.SetBlendState( Graphics::BlendDisable );
    RenderCubeWirePSO.SetDepthStencilState( Graphics::DepthStateDisabled );
    RenderCubeWirePSO.Finalize();
}

void Utility::Shutdown( void )
{
    DebugTexturePSO.Destroy();
    RenderCubePSO.Destroy();
    RenderCubeWirePSO.Destroy();
}

void Utility::DebugTexture( GraphicsContext& Context, D3D11_SRV_HANDLE SRV, LONG X, LONG Y, LONG W, LONG H )
{
    ScopedTimer _prof( L"Debug Texture", Context );

    D3D11_VIEWPORT Viewport = { (FLOAT)X, (FLOAT)Y, (FLOAT)W, (FLOAT)H };
    D3D11_RECT Scissor = { X, Y, X+W, Y+H };
    Context.SetDynamicSampler( 0, Graphics::SamplerLinearClamp, { kBindPixel} );
    Context.SetViewportAndScissor( Viewport, Scissor );
    Context.SetPipelineState( DebugTexturePSO );
    Context.SetDynamicDescriptor( 0, SRV, { kBindPixel });
    Context.Draw(3);
    Context.SetDynamicDescriptor( 0, nullptr, { kBindPixel });
}

//
// Frustum::m_FrustumCorners
//
void Utility::DebugCube( GraphicsContext& Context, const Math::Matrix4& WorldToClip, void* VertexData, Color Color )
{
    UINT16 Indices[] = {
        // Face
        0, 2, 3,
        0, 3, 1,

        // Left
        0, 1, 5,
        0, 5, 4,

        // Right
        2, 6, 7,
        2, 7, 3,

        // Bottom
        0, 4, 6,
        0, 6, 2, 

        // Top
        1, 3, 7,
        1, 7, 5,

        // Back
        4, 5, 7,
        4, 7, 6
    };

    VertexBuffer VB;
    VB.Create( L"Cube Vertex Buffer", 8, sizeof( Math::Vector3 ), VertexData );

    IndexBuffer IB;
    IB.Create( L"Cube Index Buffer", _countof( Indices ), sizeof( UINT16 ), Indices );

    Context.SetDynamicConstantBufferView( 0, sizeof(Math::Matrix4), &WorldToClip, { kBindVertex } );
    Context.SetDynamicConstantBufferView( 0, sizeof(Color), &Color.ToSRGB(), { kBindPixel } );
    Context.SetVertexBuffer( 0, VB.VertexBufferView() );
    Context.SetIndexBuffer( IB.IndexBufferView() );
    Context.SetPipelineState( RenderCubePSO );
    Context.DrawIndexed( _countof( Indices ) );
    Context.SetPipelineState( RenderCubeWirePSO );
    Context.DrawIndexed( _countof( Indices ) );

    VB.Destroy();
    IB.Destroy();
}
