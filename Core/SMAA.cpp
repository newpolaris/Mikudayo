#include "pch.h"
#include "SMAA.h"

#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"
#include "FxManager.h"
#include "FxTechnique.h"
#include "FxTechniqueSet.h"
#include "InputLayout.h"
#include "TextureManager.h"
#include "SearchTex.h"
#include "AreaTex.h"
#include "LinearColor.h"

#include "CompiledShaders/ScreenQuadVS.h"
#include "CompiledShaders/RemoveGammaPS.h"

using namespace Graphics;

namespace
{
    struct Vertex {
        XMFLOAT3 position;
        XMFLOAT2 texcoord;
    };

    std::vector<InputDesc> m_InputLayout = {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D10_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    class FullscreenTriangle {
    public:
        FullscreenTriangle();
        ~FullscreenTriangle();
        void Create( void );
        void Clear( void );
        void Draw( GraphicsContext& context );

    private:
        VertexBuffer m_Buffer;
    };

    FullscreenTriangle::FullscreenTriangle()
    {
    }

    FullscreenTriangle::~FullscreenTriangle() 
    {
        Clear();
    }

    void FullscreenTriangle::Create( void )
    {
        Vertex vertices[3];

        vertices[0].position = XMFLOAT3( -1.0f, -1.0f, 1.0f );
        vertices[1].position = XMFLOAT3( -1.0f, 3.0f, 1.0f );
        vertices[2].position = XMFLOAT3( 3.0f, -1.0f, 1.0f );

        vertices[0].texcoord = XMFLOAT2( 0.0f, 1.0f );
        vertices[1].texcoord = XMFLOAT2( 0.0f, -1.0f );
        vertices[2].texcoord = XMFLOAT2( 2.0f, 1.0f );

        m_Buffer.Create( L"FullscreenTriangle", 3, sizeof(vertices[0]), vertices );
    }

    void FullscreenTriangle::Clear( void )
    {
        m_Buffer.Destroy();
    }

    void FullscreenTriangle::Draw(GraphicsContext& context) {
        const UINT offset = 0;
        const UINT stride = sizeof( Vertex );
        context.SetVertexBuffer( 0, m_Buffer.VertexBufferView() );
        context.Draw( 3, 0 );
    }
}

namespace SMAA 
{
    BoolVar Enable("Graphics/AA/SMAA/Enable", false);

    GraphicsPSO m_RemoveGammaPSO;
    FullscreenTriangle m_Triangle;
    Texture m_AreaTexture;
    Texture m_SearchTexture;
    FxTechniquePtr m_EdgeDetection;
    FxTechniquePtr m_BlendingWeightCalculation;
    FxTechniquePtr m_NeighborhoodBlending;
}

void SMAA::Initialize( void )
{
    FxInfo fx { "SMAA", L"Shaders/SMAA.fx" };
    FxManager::Load( fx );
    FxTechniqueSetPtr techniques = FxManager::GetTechniques( "SMAA" );
    m_EdgeDetection = techniques->RequestTechnique( "LumaEdgeDetection", m_InputLayout );
    m_BlendingWeightCalculation = techniques->RequestTechnique( "BlendingWeightCalculation", m_InputLayout );
    m_NeighborhoodBlending = techniques->RequestTechnique( "NeighborhoodBlending", m_InputLayout );

    m_Triangle.Create();
    m_AreaTexture.Create( AREATEX_WIDTH, AREATEX_HEIGHT, DXGI_FORMAT_R8G8_UNORM, areaTexBytes );
    m_SearchTexture.Create( AREATEX_WIDTH, AREATEX_HEIGHT, DXGI_FORMAT_R8_UNORM, searchTexBytes );

    m_RemoveGammaPSO.SetVertexShader( MY_SHADER_ARGS(g_pScreenQuadVS) );
    m_RemoveGammaPSO.SetPixelShader( MY_SHADER_ARGS(g_pRemoveGammaPS) );
    m_RemoveGammaPSO.Finalize();
}

void SMAA::Shutdown( void )
{
    m_Triangle.Clear();
    m_EdgeDetection.reset();
    m_BlendingWeightCalculation.reset();
    m_NeighborhoodBlending.reset();

    m_AreaTexture.Destroy();
    m_SearchTexture.Destroy();
}

void SMAA::Render(ComputeContext& Context )
{
    ScopedTimer _prof( L"SMAA", Context );

    GraphicsContext& gfxContext = Context.GetGraphicsContext();

    // linear color source
    ColorBuffer& Source = Gamma::bSRGB ? g_SceneColorBuffer : g_PreviousColorBuffer;
    gfxContext.SetViewportAndScissor( 0, 0, Source.GetWidth(), Source.GetHeight() );
    if (!Gamma::bSRGB)
    {
        gfxContext.SetDynamicDescriptor( 0, g_SceneColorBuffer.GetSRV(), { kBindPixel } );
        gfxContext.SetRenderTarget( g_PreviousColorBuffer.GetRTV() );
        gfxContext.SetPipelineState( m_RemoveGammaPSO );
        gfxContext.Draw( 3 );
        gfxContext.SetRenderTarget( nullptr );
        gfxContext.SetDynamicDescriptor( 0, nullptr, { kBindPixel } );
    }

    gfxContext.ClearColor( g_SMAAEdge );
    gfxContext.ClearColor( g_SMAABlend );

    D3D11_SRV_HANDLE precomputed[] = { m_AreaTexture.GetSRV(), m_SearchTexture.GetSRV() };
    gfxContext.SetDynamicDescriptors( 0, 2, precomputed, { kBindPixel } );

    auto draw = [&]( GraphicsContext& context ) { m_Triangle.Draw( context ); };
    gfxContext.SetRenderTarget( g_SMAAEdge.GetRTV(), nullptr );
    gfxContext.SetDynamicDescriptor( 2, Source.GetSRV(), { kBindPixel } );
    m_EdgeDetection->Render( gfxContext, draw );

    gfxContext.SetRenderTarget( g_SMAABlend.GetRTV(), nullptr );
    gfxContext.SetDynamicDescriptor( 3, g_SMAAEdge.GetSRV(), { kBindPixel } );
    m_BlendingWeightCalculation->Render( gfxContext, draw );

    gfxContext.SetDynamicDescriptor( 2, g_SceneColorBuffer.GetSRV(), { kBindPixel } );
    gfxContext.SetRenderTarget( g_PreviousColorBuffer.GetRTV(), nullptr );
    gfxContext.SetDynamicDescriptor( 4, g_SMAABlend.GetSRV(), { kBindPixel } );
    m_NeighborhoodBlending->Render( gfxContext, draw );
}