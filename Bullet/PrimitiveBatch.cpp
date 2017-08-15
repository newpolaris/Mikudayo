//
// Use code from 'http://zerogram.info/'
// Bullet primitive mesh data ("Mesh/*.hpp") and color and shader ("BulletPrimitive*.hlsl")
//
// Original Boilerplate:
/*
    Program source code are licensed under the zlib license, except source code of external library.

    Zerogram Sample Program
    http://zerogram.info/

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it freely,
    subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/
#include "PrimitiveBatch.h"

#include "GameCore.h"
#include "GpuBuffer.h"
#include "GraphicsCore.h"
#include "CommandContext.h"
#include "InputLayout.h"
#include "PipelineState.h"
#include "TextureManager.h"
#include "LinearMath.h"
#include "IRigidBody.h"
#include "Math/BoundingSphere.h"
#include "Math/Frustum.h"

#include "CompiledShaders/BulletPrimitiveVS.h"
#include "CompiledShaders/BulletPrimitivePS.h"

namespace mesh_sphere {
#include "Mesh/sphere.hpp"
}
namespace mesh_ground {
#include "Mesh/ground.hpp"
}
namespace mesh_box {
#include "Mesh/box.hpp"
}
namespace mesh_cylinder {
#include "Mesh/cylinder.hpp"
}
namespace mesh_capsule {
#include "Mesh/capsule.hpp"
}
namespace mesh_cone {
#include "Mesh/cone.hpp"
}

using namespace Math;
using namespace Physics;

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

    InputDesc Layout[] = {
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

    {
        using namespace mesh_sphere;
        m_Mesh[kSphereShape] = append( mesh_vertex, vertex_num, mesh_index, index_num );
    }
    {
        using namespace mesh_ground;
        m_Mesh[kPlaneShape] = append( mesh_vertex, vertex_num, mesh_index, index_num );
    }
    {
        using namespace mesh_box;
        m_Mesh[kBoxShape] = append( mesh_vertex, vertex_num, mesh_index, index_num );
    }
    {
        using namespace mesh_capsule;
        m_Mesh[kCapsuleShape] = append( mesh_vertex, vertex_num, mesh_index, index_num );
    }
    {
        using namespace mesh_cone;
        m_Mesh[kConeShape] = append( mesh_vertex, vertex_num, mesh_index, index_num );
    }
    {
        using namespace mesh_cylinder;
        m_Mesh[kCylinderShape] = append( mesh_vertex, vertex_num, mesh_index, index_num );
    }

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
    m_PrimitivePSO.Destroy();
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

void PrimitiveBatch::Flush( GraphicsContext& gfxContext )
{
    gfxContext.SetPipelineState( m_PrimitivePSO );
	gfxContext.SetVertexBuffer( 0, m_GeometryVertexBuffer.VertexBufferView() );
	gfxContext.SetIndexBuffer( m_GeometryIndexBuffer.IndexBufferView() );
    gfxContext.SetDynamicDescriptor( 0, m_MatTexture->GetSRV(), { kBindPixel } );

    const UINT chuckMax = 64;
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
