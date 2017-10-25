#include "stdafx.h"
#include "BaseMaterial.h"
#include "BaseMesh.h"
#include "BaseModel.h"
#include "Visitor.h"

#include "CompiledShaders/ModelColorVS.h"
#include "CompiledShaders/ModelColorPS.h"
#include "CompiledShaders/ModelColor2PS.h"

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

    RenderPipelineList Default;
    RenderPipelinePtr DepthPSO, ShadowPSO, OpaquePSO, TransparentPSO;
    
#if 0
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

    OpaquePSO = std::make_shared<GraphicsPSO>();
    // *OpaquePSO = *DepthPSO;
    OpaquePSO->SetInputLayout( _countof( VertElem ), VertElem );
    OpaquePSO->SetBlendState( BlendDisable );
    OpaquePSO->SetRasterizerState( RasterizerDefault );
    OpaquePSO->SetDepthStencilState( DepthStateReadWrite );
    OpaquePSO->SetVertexShader( MY_SHADER_ARGS( g_pModelColorVS ) );
    OpaquePSO->SetPixelShader( MY_SHADER_ARGS( g_pModelColorPS ) );;
    OpaquePSO->Finalize();

    AutoFillPSO( OpaquePSO, kRenderQueueOpaque, Default );

    RenderPipelinePtr ReflectedPSO = std::make_shared<GraphicsPSO>();
    *ReflectedPSO = *OpaquePSO;
    ReflectedPSO->SetPixelShader( MY_SHADER_ARGS( g_pModelColor2PS ) );
    ReflectedPSO->SetRasterizerState( RasterizerDefaultCW );
    ReflectedPSO->Finalize();

    AutoFillPSO( ReflectedPSO, kRenderQueueReflectOpaque, Default );

    Techniques.emplace( L"Default", std::move( Default ) );
}

void BaseModel::Shutdown()
{
    Techniques.clear();
}

RenderPipelinePtr BaseMaterial::GetPipeline( RenderQueue Queue ) 
{
    return Techniques[L"Default"][Queue];
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

bool BaseModel::Load( const ModelInfo& info )
{
    m_FileName = info.ModelFile;
    m_DefaultShader = info.DefaultShader;

    return true;
}

void BaseModel::Render( GraphicsContext& gfxContext, Visitor& visitor )
{
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

Math::Matrix4 BaseModel::GetTransform() const
{
    return m_Transform;
}

void BaseModel::SetTransform( const Math::Matrix4& transform )
{
    m_Transform = transform;
}
