#include "stdafx.h"
#include "BaseModel.h"
#include "Visitor.h"

#include "CompiledShaders/ModelColorVS.h"
#include "CompiledShaders/ModelColorPS.h"

namespace {
    std::map<std::wstring, RenderPipelineList> Techniques;

    InputDesc VertElem[]
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
}

void BaseModel::Initialize()
{
    using namespace Graphics;

#if 0
    RenderPipelinePtr DeferredGBufferPSO, OutlinePSO, DepthPSO, ShadowPSO;

    DeferredGBufferPSO = std::make_shared<GraphicsPSO>();
    DeferredGBufferPSO->SetInputLayout( _countof(VertElem), VertElem );
    DeferredGBufferPSO->SetVertexShader( MY_SHADER_ARGS( g_pColorVS ) );
    DeferredGBufferPSO->SetPixelShader( MY_SHADER_ARGS( g_pDeferredGBufferPS ) );
    DeferredGBufferPSO->SetDepthStencilState( DepthStateReadWrite );
    DeferredGBufferPSO->SetRasterizerState( RasterizerDefault );
    DeferredGBufferPSO->Finalize();

    OutlinePSO = std::make_shared<GraphicsPSO>();
    OutlinePSO->SetInputLayout( _countof(VertElem), VertElem );
    OutlinePSO->SetVertexShader( MY_SHADER_ARGS( g_pOutlineVS ) );
    OutlinePSO->SetPixelShader( MY_SHADER_ARGS( g_pOutlinePS ) );
    OutlinePSO->SetRasterizerState( RasterizerDefaultCW );
    OutlinePSO->SetDepthStencilState( DepthStateReadWrite );
    OutlinePSO->Finalize();

    DXGI_FORMAT DepthFormat = g_SceneDepthBuffer.GetFormat();

    DepthPSO = std::make_shared<GraphicsPSO>();
    DepthPSO->SetInputLayout( _countof(VertElem), VertElem );
    DepthPSO->SetRenderTargetFormats( 0, nullptr, DepthFormat );
    DepthPSO->SetVertexShader( MY_SHADER_ARGS( g_pDepthViewerVS ) );
    DepthPSO->SetBlendState( BlendNoColorWrite );
    DepthPSO->SetRasterizerState( RasterizerDefault );
    DepthPSO->SetDepthStencilState( DepthStateReadWrite );
    DepthPSO->Finalize();

    ShadowPSO = std::make_shared<GraphicsPSO>();
    *ShadowPSO = *DepthPSO;
    ShadowPSO->SetRasterizerState( RasterizerShadow );
    ShadowPSO->SetRenderTargetFormats( 0, nullptr, g_ShadowBuffer.GetFormat() );
    ShadowPSO->Finalize();
#endif

    RenderPipelineList Default;
    RenderPipelinePtr OpaquePSO, TransparentPSO, GBufferPSO, FinalPSO;
    {
        OpaquePSO = std::make_shared<GraphicsPSO>();
        // *OpaquePSO = *DepthPSO;
        OpaquePSO->SetInputLayout( _countof( VertElem ), VertElem );
        OpaquePSO->SetBlendState( BlendDisable );
        OpaquePSO->SetRasterizerState( RasterizerDefault );
        OpaquePSO->SetDepthStencilState( DepthStateReadWrite );
        OpaquePSO->SetVertexShader( MY_SHADER_ARGS( g_pModelColorVS ) );
        OpaquePSO->SetPixelShader( MY_SHADER_ARGS( g_pModelColorPS ) );;
        OpaquePSO->Finalize();

        TransparentPSO = std::make_shared<GraphicsPSO>();
        *TransparentPSO = *OpaquePSO;
        TransparentPSO->SetBlendState( BlendTraditional );
        TransparentPSO->Finalize();

    #if 0
        FinalPSO = std::make_shared<GraphicsPSO>();
        FinalPSO->SetInputLayout( (UINT)Pmx::VertElem.size(), Pmx::VertElem.data() );
        FinalPSO->SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
        FinalPSO->SetPixelShader( MY_SHADER_ARGS( g_pDeferredFinalPS ) );
        FinalPSO->SetRasterizerState( RasterizerDefault );
        FinalPSO->SetDepthStencilState( DepthStateTestEqual );
        FinalPSO->Finalize();

        GBufferPSO = std::make_shared<GraphicsPSO>();
        GBufferPSO->SetInputLayout( (UINT)Pmx::VertElem.size(), Pmx::VertElem.data() );
        GBufferPSO->SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
        GBufferPSO->SetPixelShader( MY_SHADER_ARGS( g_pDeferredGBufferPS ) );
        GBufferPSO->SetDepthStencilState( DepthStateReadWrite );
        GBufferPSO->SetRasterizerState( RasterizerDefault );
        GBufferPSO->Finalize();
#endif
        
        Default[kRenderQueueOpaque] = OpaquePSO;
        Default[kRenderQueueTransparent] = TransparentPSO;
        Default[kRenderQueueDeferredGBuffer] = GBufferPSO;
        Default[kRenderQueueDeferredFinal] = FinalPSO;
        // Default[kRenderQueueOutline] = OutlinePSO;
        // Default[kRenderQueueShadow] = ShadowPSO;
    }
    Techniques.emplace( L"Default", std::move( Default ) );
}

void BaseModel::Shutdown()
{
    Techniques.clear();
}

bool BaseMaterial::IsTransparent() const
{
    bool bHasTransparentTexture = textures[kDiffuse] && textures[kDiffuse]->IsTransparent();
    return opacity < 1.0f || bHasTransparentTexture;
}

RenderPipelinePtr BaseMaterial::GetPipeline( RenderQueue Queue ) 
{
    return Techniques[L"Default"][Queue];
}

void BaseMaterial::Bind( GraphicsContext& gfxContext )
{
    D3D11_SRV_HANDLE SRV[kTexCount] = { nullptr };
    for (auto i = 0; i < _countof( textures ); i++)
    {
        if (textures[i] == nullptr) continue;
        SRV[i] = textures[i]->GetSRV();
    }
    gfxContext.SetDynamicDescriptors( 1, _countof( SRV ), SRV, { kBindPixel } );

    __declspec(align(16)) struct {
        Vector3 diffuse;
        Vector3 specular;
        Vector3 ambient;
        Vector3 emissive;
        Vector3 transparent; // light passing through a transparent surface is multiplied by this filter color
        float opacity;
        float shininess; // specular exponent
        float specularStrength; // multiplier on top of specular color
        uint32_t bTexture[kTexCount];
    } material = {
        diffuse, specular, ambient, emissive, transparent, opacity, shininess, specularStrength
    };
    for (auto i = 0; i < _countof( textures ); i++)
        material.bTexture[i] = (SRV[i] == nullptr ? 0 : 1);
    gfxContext.SetDynamicConstantBufferView( 4, sizeof( material ), &material, { kBindVertex, kBindPixel } );
}

BaseModel::BaseModel() : m_DefaultShader(L"Default")
{
    m_Transform = Matrix4::MakeScale( 10.f );
}

void BaseModel::Clear()
{
    m_VertexBuffer.Destroy();
    m_IndexBuffer.Destroy();
    m_VertexBufferDepth.Destroy();
    m_IndexBufferDepth.Destroy();
}

void BaseModel::Render( GraphicsContext& gfxContext, Visitor& visitor )
{
    gfxContext.SetDynamicConstantBufferView( 2, sizeof(m_Transform), &m_Transform, { kBindVertex } );
	gfxContext.SetVertexBuffer( 0, m_VertexBuffer.VertexBufferView() );
	gfxContext.SetIndexBuffer( m_IndexBuffer.IndexBufferView() );

    for (auto& mesh : m_Meshes)
	{
        if (!visitor.Visit( *mesh ))
            continue;
        MaterialPtr& material = mesh->material;
        if (!visitor.Visit( *material ))
            continue;
        gfxContext.DrawIndexed( mesh->indexCount, mesh->startIndex, mesh->baseVertex );
	}
}

bool BaseModel::Load( const ModelInfo& info )
{
    m_FileName = info.ModelFile;
    m_DefaultShader = info.DefaultShader;

    return true;
}