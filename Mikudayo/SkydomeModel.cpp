#include "stdafx.h"
#include "SkydomeModel.h"
#include "LinearColor.h"
#include "BaseMaterial.h"
#include "BaseMesh.h"
#include "BaseModel.h"
#include "GeometryGenerator.h"

#include "CompiledShaders/SkydomeColorVS.h"
#include "CompiledShaders/SkydomeColorPS.h"

namespace
{
    const InputDesc VertElem[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
}

SkydomeModel::SkydomeModel()
{
    m_DefaultShader = L"Skydome";
}

void SkydomeModel::Initialize()
{
    using namespace Graphics;

    RenderPipelineList Skydome;
    RenderPipelinePtr SkydomePSO = std::make_shared<GraphicsPSO>();
    D3D11_RASTERIZER_DESC Raster = RasterizerDefault;
    Raster.CullMode = D3D11_CULL_FRONT;
    SkydomePSO->SetPrimitiveTopologyType( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    SkydomePSO->SetInputLayout( _countof( VertElem ), VertElem );
    SkydomePSO->SetVertexShader( MY_SHADER_ARGS( g_pSkydomeColorVS ) );
    SkydomePSO->SetPixelShader( MY_SHADER_ARGS( g_pSkydomeColorPS ) );
    SkydomePSO->SetBlendState( BlendDisable );
    SkydomePSO->SetRasterizerState( Raster );
    SkydomePSO->SetDepthStencilState( DepthStateReadWrite );
    SkydomePSO->Finalize();
    Skydome[kRenderQueueSkydome] = SkydomePSO;
    BaseModel::AppendTechniques( L"Skydome", std::move( Skydome ) );
}

void SkydomeModel::Shutdown()
{
}

bool SkydomeModel::Load( const ModelInfo& info )
{
    BaseModel::Load( info );

    auto material = std::make_shared<BaseMaterial>();
    const ManagedTexture* texture = TextureManager::LoadFromFile( info.ModelFile, Gamma::bSRGB );
    material->textures[BaseMaterial::kDiffuse] = texture;
    material->shader = m_DefaultShader;
    m_Materials.emplace_back(material);

    GeometryGenerator::MeshData sphere = GeometryGenerator().CreateSphere( 5000, 16, 16 );
    auto indices = sphere.GetIndices16();
    auto vertices = sphere.Vertices;

    auto mesh = std::make_shared<BaseMesh>();
    mesh->material = material;
    mesh->materialIndex = 0;
    mesh->indexCount = uint32_t(indices.size());
    mesh->vertexCount = uint32_t(vertices.size());
    mesh->vertexStride = sizeof(vertices[0]);
    m_Meshes.emplace_back( mesh );

    m_IndexBuffer.Create( L"Skydome IndexBuffer", mesh->indexCount, 2, indices.data() );
    m_VertexBuffer.Create( L"Skydome VertexBuffer", mesh->vertexCount, mesh->vertexStride, vertices.data() );

    return true;
}