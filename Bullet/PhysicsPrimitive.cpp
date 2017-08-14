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

#include "PhysicsPrimitive.h"
#include "Physics.h"
#include "BaseRigidBody.h"
#include "LinearMath.h"

#include "GameCore.h"
#include "GpuBuffer.h"
#include "GraphicsCore.h"
#include "CommandContext.h"
#include "InputLayout.h"
#include "PipelineState.h"
#include "TextureManager.h"

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

namespace Primitive
{
	struct SubmeshGeometry
	{
		uint32_t IndexCount;
		int32_t IndexOffset;
		int32_t VertexOffset;
	};

    const ManagedTexture* m_MatTexture = nullptr;
    Color m_Color[kMaxShapeType];
    SubmeshGeometry m_Mesh[kMaxShapeType];
    VertexBuffer m_GeometryVertexBuffer;
    IndexBuffer m_GeometryIndexBuffer;
    GraphicsPSO m_PrimitivePSO;

    void Initialize();
    void Shutdown();
    PhysicsPrimitivePtr CreatePhysicsPrimitive( const PhysicsPrimitiveInfo& Info );
}

using namespace Physics;
using namespace Primitive;

void Primitive::Initialize()
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

        return SubmeshGeometry { NumIndices, baseIndex, baseVertex };
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

}

void Primitive::Shutdown()
{
    m_PrimitivePSO.Destroy();
}

PhysicsPrimitivePtr Primitive::CreatePhysicsPrimitive( const PhysicsPrimitiveInfo& Info )
{
    auto Ptr = std::make_shared<PhysicsPrimitive>();
    if (Ptr)
        Ptr->Create( Info );
    return Ptr;
}

PhysicsPrimitive::PhysicsPrimitive() : m_Type(kUnknownShape)
{
}

void PhysicsPrimitive::Create( const PhysicsPrimitiveInfo& Info )
{
    m_Type = Info.Type;
    m_Kind = Info.Mass > 0.f ? kDynamicObject : kStaticObject;

    auto Body = std::make_shared<BaseRigidBody>();
    Body->SetObjectType( m_Kind );
    Body->SetShapeType( m_Type );
    Body->SetMass( Info.Mass );
    Body->SetPosition( Convert(Info.Position) );
    Body->SetSize( Convert(Info.Size) );
    Body->Build();
    Body->JoinWorld( g_DynamicsWorld );

    m_Body.swap( Body );
}

void PhysicsPrimitive::Destroy()
{
    m_Body->LeaveWorld( g_DynamicsWorld );
    m_GeometryIndexBuffer.Destroy();
    m_GeometryVertexBuffer.Destroy();
}

void PhysicsPrimitive::Draw( GraphicsContext& gfxContext )
{
    auto GetScale = [](ShapeType Type, btVector3 Vec) {
        switch (Type) {
        case kSphereShape:
            Vec.setY( Vec.x() );
            Vec.setZ( Vec.x() );
            break;
        case kCapsuleShape:
        case kConeShape:
            Vec.setZ( Vec.x() );
            Vec = Vec * btVector3( 1, 0.5f, 1 );
            break;
        case kPlaneShape:
            Vec = btVector3( 1, 1, 1 );
            break;
        }
        return Convert(Vec);
    };

    Vector3 scaleVec = GetScale( m_Type, m_Body->GetSize());
    AffineTransform scale = AffineTransform::MakeScale(scaleVec);
    Matrix4 transform = Convert(m_Body->GetTransfrom()) * scale;

    struct {
        Color Diffuse;
        uint32_t bTexture;
    } psConstants {
        m_Color[m_Type],
        TRUE,
    };
    gfxContext.SetDynamicDescriptor( 0, m_MatTexture->GetSRV(), { kBindPixel } );
    gfxContext.SetDynamicConstantBufferView( 1, sizeof( psConstants ), &psConstants, { kBindPixel } );
    auto draw = [&]( auto& Matrix, auto& Mesh ) {
        gfxContext.SetDynamicConstantBufferView( 1, sizeof( Matrix ), &Matrix, { kBindVertex } );
        gfxContext.DrawIndexed( Mesh.IndexCount, Mesh.IndexOffset, Mesh.VertexOffset );
    };
    gfxContext.SetPipelineState( m_PrimitivePSO );
	gfxContext.SetVertexBuffer( 0, m_GeometryVertexBuffer.VertexBufferView() );
	gfxContext.SetIndexBuffer( m_GeometryIndexBuffer.IndexBufferView() );

    if (m_Type != kCapsuleShape)
    {
        draw( transform, m_Mesh[m_Type] );
    }
    else
    {
        auto radius = m_Body->GetSize().x();
        auto height = m_Body->GetSize().y();
        auto capScale = AffineTransform::MakeScale( Vector3( radius ) );
        auto topOffset = AffineTransform::MakeTranslation( Vector3(0, height/2, 0) );
        auto bottomOffset = AffineTransform::MakeTranslation( Vector3(0, -height/2, 0) );
        Matrix4 top = Convert( m_Body->GetTransfrom() ) * topOffset * capScale;
        Matrix4 bottom = Convert( m_Body->GetTransfrom() ) * bottomOffset * capScale;
        draw( transform, m_Mesh[kCylinderShape] );
        draw( top, m_Mesh[kCapsuleShape] );
        draw( bottom, m_Mesh[kCapsuleShape] );
    }
}