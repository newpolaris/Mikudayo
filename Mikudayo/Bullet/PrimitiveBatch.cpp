#include "stdafx.h"
#include "PrimitiveMesh.h"
#include "PrimitiveBatch.h"
#include "LinearMath.h"
#include "IRigidBody.h"

#include "Math/BoundingSphere.h"
#include "Math/Frustum.h"

#include "CompiledShaders/BulletPrimitiveVS.h"
#include "CompiledShaders/BulletPrimitivePS.h"

using namespace Math;

namespace PrimitiveBatch
{
    enum BatchDraw
    {
        kBatchSphere,
        kBatchBox,
        kBatchCapsule,
        kBatchCone,
        kBatchCylinder,
        kBatchPlane,
        kBatchCapsuleBody,
        kBatchMax
    };

	struct SubmeshGeometry
	{
		uint32_t IndexCount;
		int32_t IndexOffset;
		int32_t VertexOffset;
        Math::BoundingSphere Bound;
	};

    const ManagedTexture* m_MatTexture = nullptr;

    Color m_Color[kBatchMax];
    SubmeshGeometry m_Mesh[kBatchMax];
    VertexBuffer m_GeometryVertexBuffer;
    IndexBuffer m_GeometryIndexBuffer;
    GraphicsPSO m_PrimitivePSO;
    std::vector<Matrix4> m_PrimitiveQueue[kBatchMax];
}

void PrimitiveBatch::Initialize()
{
    m_MatTexture = TextureManager::LoadFromFile("check.dds", true);

    const InputDesc Layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    m_PrimitivePSO.SetInputLayout( _countof(Layout), Layout );
    m_PrimitivePSO.SetVertexShader( MY_SHADER_ARGS( g_pBulletPrimitiveVS ) );
    m_PrimitivePSO.SetPixelShader( MY_SHADER_ARGS( g_pBulletPrimitivePS ) );
    m_PrimitivePSO.SetDepthStencilState( Graphics::DepthStateReadWrite );
    m_PrimitivePSO.SetRasterizerState( Graphics::RasterizerDefault );
    m_PrimitivePSO.Finalize();

    struct Vertex
    {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT2 Texcoord;
    };
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

    auto append = [&]( float* Vertices, int32_t NumVertex, UINT* Indices, uint32_t NumIndices )
    {
        Vertex* pVertices = reinterpret_cast<Vertex*>(Vertices);
        std::vector<Vertex> vertex(pVertices, pVertices+NumVertex);
        for (auto& v : vertex) {
            v.Position.z = -v.Position.z;
            v.Normal.z = -v.Normal.z;
        }

        std::vector<UINT> index(Indices, Indices+NumIndices);
        for (uint32_t i = 0; i < NumIndices; i += 3)
            std::swap( index[i], index[i + 1] );

        int32_t baseIndex = static_cast<int32_t>(indices.size());
        int32_t baseVertex = static_cast<int32_t>(vertices.size());
        std::copy( vertex.begin(), vertex.end(), std::back_inserter( vertices ) );
        std::copy( index.begin(), index.end(), std::back_inserter( indices ) );

        // bounding sphere
        std::vector<XMFLOAT3> positions(vertex.size());
        for (auto i = 0; i < vertex.size(); i++)
            positions[i] = vertex[i].Position;
        auto sphere = Math::ComputeBoundingSphereFromVertices(positions);

        return SubmeshGeometry { NumIndices, baseIndex, baseVertex, sphere };
    };

    m_Mesh[kSphereShape] = append( mesh_sphere::mesh_vertex, mesh_sphere::vertex_num, mesh_sphere::mesh_index, mesh_sphere::index_num );
    m_Mesh[kPlaneShape] = append( mesh_ground::mesh_vertex, mesh_ground::vertex_num, mesh_ground::mesh_index, mesh_ground::index_num );
    m_Mesh[kBoxShape] = append( mesh_box::mesh_vertex, mesh_box::vertex_num, mesh_box::mesh_index, mesh_box::index_num );
    m_Mesh[kCapsuleShape] = append( mesh_capsule::mesh_vertex, mesh_capsule::vertex_num, mesh_capsule::mesh_index, mesh_capsule::index_num );
    m_Mesh[kConeShape] = append( mesh_cone::mesh_vertex, mesh_cone::vertex_num, mesh_cone::mesh_index, mesh_cone::index_num );
    m_Mesh[kCylinderShape] = append( mesh_cylinder::mesh_vertex, mesh_cylinder::vertex_num, mesh_cylinder::mesh_index, mesh_cylinder::index_num );

	m_GeometryVertexBuffer.Create( L"PhysicsVertex", static_cast<uint32_t>(vertices.size()), sizeof(Vertex), vertices.data() );
	m_GeometryIndexBuffer.Create( L"PhysicsIndex", static_cast<uint32_t>(indices.size()), sizeof(uint32_t), indices.data() );

    m_Color[kSphereShape] = Color( 1, 0.7f, 0.7f ).FromSRGB();
    m_Color[kCylinderShape] = Color( 0.7f, 1.0f, 0.7f ).FromSRGB();
    m_Color[kConeShape] = Color( 1.0f, 1.0f, 0.7f ).FromSRGB();
    m_Color[kCapsuleShape] = Color( 0.5f, 0.7f, 1.0f ).FromSRGB();
    m_Color[kPlaneShape] = Color( 1.0f, 0.875f, 0.64f ).FromSRGB();
    m_Color[kBoxShape] = Color( 0.95f, 1.0f, 1.0f ).FromSRGB();
    m_Color[kBatchCapsuleBody] = m_Color[kBatchCapsule];
    m_Mesh[kBatchCapsuleBody] = m_Mesh[kBatchCylinder];
}

void PrimitiveBatch::Shutdown()
{
    m_GeometryIndexBuffer.Destroy();
    m_GeometryVertexBuffer.Destroy();
}

void PrimitiveBatch::Append( ShapeType Type,
    const AffineTransform& Transform, const Vector3& Size, const Frustum& CameraFrustum )
{
    auto GetScale = [](ShapeType Type, Vector3 Vec) {
        switch (Type) {
        case kSphereShape:
            Vec.SetY( Vec.GetX() );
            Vec.SetZ( Vec.GetX() );
            return Vec;
        case kCapsuleShape:
        case kConeShape:
            Vec.SetZ( Vec.GetX() );
            return Vec * Vector3( 1, 0.5f, 1 );
        case kPlaneShape:
            return Vector3( 1, 1, 1 );
        }
        return Vec;
    };

    Vector3 scaleVec = GetScale( Type, Size );
    AffineTransform transform = Transform * AffineTransform::MakeScale(scaleVec);

    if (Type != kBatchCapsule)
    {
        BoundingSphere transformed = transform * m_Mesh[Type].Bound;
        if (CameraFrustum.IntersectSphere(transformed))
            m_PrimitiveQueue[Type].push_back( transform );
    }
    else
    {
        auto radius = Size.GetX();
        auto height = Size.GetY();

        // Roughly setting bounding radius
        BoundingSphere transformed = transform * BoundingSphere(Vector3(kZero), radius + height );
        if (!CameraFrustum.IntersectSphere(transformed))
            return;

        auto capScale = AffineTransform::MakeScale( Vector3( radius ) );
        auto topOffset = AffineTransform::MakeTranslation( Vector3(0, height/2.f, 0) );
        auto bottomOffset = AffineTransform::MakeTranslation( Vector3(0, -height/2.f, 0) );

        Matrix4 top = Transform * topOffset * capScale;
        Matrix4 bottom = Transform * bottomOffset * capScale;

        m_PrimitiveQueue[kBatchCapsuleBody].push_back( transform );
        m_PrimitiveQueue[kBatchCapsule].push_back( top );
        m_PrimitiveQueue[kBatchCapsule].push_back( bottom );
    }
}

void PrimitiveBatch::Flush( GraphicsContext& gfxContext, const Math::Matrix4& WorldToClip )
{
    struct {
        Math::Matrix4 WorldToClip;
    } vsConstant;
    vsConstant.WorldToClip = WorldToClip;
    gfxContext.SetDynamicConstantBufferView( 0, sizeof( vsConstant ), &vsConstant, { kBindVertex } );
    gfxContext.SetPipelineState( m_PrimitivePSO );
	gfxContext.SetVertexBuffer( 0, m_GeometryVertexBuffer.VertexBufferView() );
	gfxContext.SetIndexBuffer( m_GeometryIndexBuffer.IndexBufferView() );
    gfxContext.SetDynamicDescriptor( 0, m_MatTexture->GetSRV(), { kBindPixel } );

    const UINT chuckMax = 1024;
    for (auto i = 0; i < kBatchMax; i++)
    {
        Color& color = m_Color[i];
        SubmeshGeometry& mesh = m_Mesh[i];
        UINT queueLength = static_cast<UINT>(m_PrimitiveQueue[i].size());
        gfxContext.SetDynamicConstantBufferView( 1, sizeof( Color ), &color, { kBindPixel } );
        for (UINT k = 0; k < queueLength; k += chuckMax)
        {
            int chuck = std::min(chuckMax, queueLength - k);
            uint32_t buffsize = sizeof( Matrix4 ) * chuck;
            void* pointer = m_PrimitiveQueue[i].data() + k;
            gfxContext.SetDynamicConstantBufferView( 1, buffsize, pointer, { kBindVertex } );
            gfxContext.DrawIndexedInstanced( mesh.IndexCount,
                chuck, mesh.IndexOffset, mesh.VertexOffset, k );
        }
        m_PrimitiveQueue[i].resize(0);
    }
}
