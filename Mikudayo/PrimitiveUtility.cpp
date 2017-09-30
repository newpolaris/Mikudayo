#include "stdafx.h"
#include "PrimitiveUtility.h"
#include "Math/Matrix4.h"

#include "CompiledShaders/ModelPrimitiveVS.h"
#include "CompiledShaders/ModelPrimitivePS.h"

using namespace Math;
using namespace Graphics;
using namespace Graphics::PrimitiveUtility;

namespace Graphics {
namespace PrimitiveUtility {

	InputDesc Desc[4] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

    BoolVar s_bEnableDrawBone( "Application/Model/Draw Bone", false );
    BoolVar s_bEnableDrawBoundingSphere( "Application/Model/Draw Bounding Shphere", false );
    // If model is mixed with sky box, model's boundary is exculde by 's_ExcludeRange'
    BoolVar s_bExcludeSkyBox( "Application/Model/Exclude Sky Box", true );
    NumVar s_ExcludeRange( "Application/Model/Exclude Range", 1000.f, 500.f, 10000.f );

	struct SubmeshGeometry
	{
		uint32_t IndexCount;
		int32_t IndexOffset;
		int32_t VertexOffset;
	};

    GraphicsPSO m_PrimitivePSO;
    VertexBuffer m_GeometryVertexBuffer;
    IndexBuffer m_GeometryIndexBuffer;
    SubmeshGeometry m_Mesh[kBatchMax];
    std::vector<Matrix4> m_PrimitiveQueue[kBatchMax];
};
};

void PrimitiveUtility::Initialize()
{
	std::vector<GeometryGenerator::Vertex> Vertices;
	std::vector<uint32_t> Indices;

	{
        GeometryGenerator geoGen;
        auto AppendGeometry = [&]( const GeometryGenerator::MeshData& data )
        {
            int32_t baseIndex = static_cast<int32_t>(Indices.size());
            int32_t baseVertex = static_cast<int32_t>(Vertices.size());
            std::copy( data.Vertices.begin(), data.Vertices.end(), std::back_inserter( Vertices ) );
            std::copy( data.Indices32.begin(), data.Indices32.end(), std::back_inserter( Indices ) );

            SubmeshGeometry mesh;
            mesh.IndexCount = static_cast<int32_t>(data.Indices32.size());
            mesh.IndexOffset = baseIndex;
            mesh.VertexOffset = baseVertex;

            return mesh;
        };
        m_Mesh[kBoneMesh] = AppendGeometry( geoGen.CreateCylinder( 2.0f, 0.1f, 1.0f, 5, 1 ) );
		m_Mesh[kSphereMesh] = AppendGeometry( geoGen.CreateSphere( 1.0f, 8, 8 ) );
        m_Mesh[kFarClipMesh] = AppendGeometry( geoGen.CreateQuad( -1, -1, 2, 2, -1 ) );
	}

	m_GeometryVertexBuffer.Create( L"GeometryVertex", static_cast<uint32_t>(Vertices.size()),
		sizeof( Vertices.front() ), Vertices.data() );

	m_GeometryIndexBuffer.Create( L"GeometryIndex", static_cast<uint32_t>(Indices.size()),
		sizeof( Indices.front() ), Indices.data() );

	D3D11_RASTERIZER_DESC RasterizerWire = RasterizerDefault;
	RasterizerWire.FillMode = D3D11_FILL_WIREFRAME;

	m_PrimitivePSO.SetInputLayout( _countof(Desc), Desc );
	m_PrimitivePSO.SetVertexShader( MY_SHADER_ARGS( g_pModelPrimitiveVS ) );
	m_PrimitivePSO.SetPixelShader( MY_SHADER_ARGS( g_pModelPrimitivePS ) );
	m_PrimitivePSO.SetDepthStencilState( DepthStateDisabled );
	m_PrimitivePSO.SetRasterizerState( RasterizerWire );
	m_PrimitivePSO.Finalize();
}

void PrimitiveUtility::Shutdown()
{
	m_GeometryIndexBuffer.Destroy();
	m_GeometryVertexBuffer.Destroy();
}

void PrimitiveUtility::Append( PrimtiveMeshType Type, const Math::Matrix4& Transform )
{
    m_PrimitiveQueue[Type].push_back(Transform);
}

void PrimitiveUtility::Flush( GraphicsContext& Context )
{
	Context.SetPipelineState( m_PrimitivePSO );
	Context.SetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	Context.SetVertexBuffer( 0, m_GeometryVertexBuffer.VertexBufferView() );
	Context.SetIndexBuffer( m_GeometryIndexBuffer.IndexBufferView() );

    const UINT chuckMax = 1024;
    for (auto i = 0; i < kBatchMax; i++)
    {
        SubmeshGeometry& mesh = m_Mesh[i];
        UINT queueLength = static_cast<UINT>(m_PrimitiveQueue[i].size());
        for (UINT k = 0; k < queueLength; k += chuckMax)
        {
            int chuck = std::min(chuckMax, queueLength - k);
            uint32_t buffsize = sizeof( Matrix4 ) * chuck;
            void* pointer = m_PrimitiveQueue[i].data() + k;
            Context.SetDynamicConstantBufferView( 1, buffsize, pointer, { kBindVertex } );
            Context.DrawIndexedInstanced( mesh.IndexCount,
                chuck, mesh.IndexOffset, mesh.VertexOffset, k );
        }
        m_PrimitiveQueue[i].resize(0);
    }
}

void Graphics::PrimitiveUtility::Render( GraphicsContext& Context, PrimtiveMeshType Type )
{
	Context.SetVertexBuffer( 0, m_GeometryVertexBuffer.VertexBufferView() );
	Context.SetIndexBuffer( m_GeometryIndexBuffer.IndexBufferView() );
    SubmeshGeometry& mesh = m_Mesh[Type];
    Context.DrawIndexed( mesh.IndexCount, mesh.IndexOffset, mesh.VertexOffset );
}
