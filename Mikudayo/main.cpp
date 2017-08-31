#include "stdafx.h"
#include "Model.h"
#include "PmxModel.h"
#include "Bullet/LinearMath.h"
#include "Bullet/Physics.h"
#include "Bullet/PhysicsPrimitive.h"
#include "Bullet/PrimitiveBatch.h"
#include "SoftBodyManager.h"

using namespace Math;
using namespace GameCore;
using namespace Graphics;
using namespace Math;

class Mikudayo : public GameCore::IGameApp
{
public:
	Mikudayo()
	{
	}

	virtual void Startup( void ) override;
	virtual void Cleanup( void ) override;

	virtual void Update( float deltaT ) override;
	virtual void RenderScene( void ) override;
    virtual void RenderUI( GraphicsContext& Context ) override;

private:

    Camera m_Camera;
    std::auto_ptr<CameraController> m_CameraController;

    Matrix4 m_ViewMatrix;
    Matrix4 m_ProjMatrix;
    Matrix4 m_ViewProjMatrix;
    D3D11_VIEWPORT m_MainViewport;
    D3D11_RECT m_MainScissor;

    btSoftBody* m_SoftBody;
    Rendering::PmxModel m_Mikudayo;
    std::vector<Primitive::PhysicsPrimitivePtr> m_Primitives;
};

CREATE_APPLICATION( Mikudayo )

SoftBodyManager manager;

void Mikudayo::Startup( void )
{
    TextureManager::Initialize( L"Textures" );
    Physics::Initialize();
    Model::Initialize();

    const Vector3 eye = Vector3(62.0f, 18.0f, -15.0f);
    m_Camera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
    m_CameraController.reset(new CameraController(m_Camera, Vector3(kYUnitVector)));

    Rendering::PmxModel mikudayo;
    mikudayo.LoadFromFile( L"Model/Mikudayo/mikudayo-3_6.pmx" );
    m_Mikudayo = std::move(mikudayo);

    auto vertices = m_Mikudayo.GetVertices();
    auto indices = m_Mikudayo.GetIndices();
    SoftBodyGeometry Geo { vertices, indices };
    m_SoftBody = manager.LoadFromGeometry( Geo );
    m_SoftBody->translate( btVector3( 0, 10, 0 ) );

    std::vector<Primitive::PhysicsPrimitiveInfo> primitves = {
        { Physics::kPlaneShape, 0.f, Vector3( kZero ), Vector3( kZero ) },
        { Physics::kBoxShape, 20.f, Vector3( 10, 1, 10 ), Vector3( 0, 2, 0 ) },
        { Physics::kBoxShape, 20.f, Vector3( 2,1,5 ), Vector3( 10, 2, 0 ) },
        { Physics::kBoxShape, 20.f, Vector3( 8,1,2 ), Vector3( 0, 2, 10 ) },
        { Physics::kBoxShape, 20.f, Vector3( 8,1,2 ), Vector3( 0, 2, -13 ) },
    };
    for (auto& info : primitves)
        m_Primitives.push_back( std::move( Primitive::CreatePhysicsPrimitive( info ) ) );
}

void Mikudayo::Cleanup( void )
{
    Model::Shutdown();
    for (auto& model : m_Primitives)
        model->Destroy();
    m_Primitives.clear();
    Physics::Shutdown();
}

void Mikudayo::Update( float deltaT )
{
    ScopedTimer _prof( L"Update" );

    m_CameraController->Update( deltaT );
    m_ViewMatrix = m_Camera.GetViewMatrix();
    m_ProjMatrix = m_Camera.GetProjMatrix();
    m_ViewProjMatrix = m_Camera.GetViewProjMatrix();

	m_MainViewport.Width = (float)g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height = (float)g_SceneColorBuffer.GetHeight();
	m_MainViewport.MinDepth = 0.0f;
	m_MainViewport.MaxDepth = 1.0f;

	m_MainScissor.left = 0;
	m_MainScissor.top = 0;
	m_MainScissor.right = (LONG)g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom = (LONG)g_SceneColorBuffer.GetHeight();

    if (!EngineProfiling::IsPaused())
    {
        Physics::Update( deltaT );
        for (auto& primitive : m_Primitives)
            primitive->Update();

        if (m_SoftBody)
        {
            auto& nodes = m_SoftBody->m_nodes;
            auto numNodes = nodes.size();
            std::vector<XMFLOAT3> vertices( numNodes );
            for (auto i = 0; i < numNodes; i++)
                vertices[i] = *reinterpret_cast<XMFLOAT3*>(&Convert( nodes[i].m_x ));
            m_Mikudayo.SetVertices( vertices );
        }
    }
}

void Mikudayo::RenderScene( void )
{
	GraphicsContext& gfxContext = GraphicsContext::Begin( L"Scene Render" );

    struct VSConstants
    {
        Matrix4 view;
        Matrix4 projection;
    } vsConstants;
    vsConstants.view = m_Camera.GetViewMatrix();
    vsConstants.projection = m_Camera.GetProjMatrix();
	gfxContext.SetDynamicConstantBufferView( 0, sizeof(vsConstants), &vsConstants, { kBindVertex } );

    D3D11_SAMPLER_HANDLE Sampler[] = { SamplerLinearWrap, SamplerLinearClamp, SamplerShadow };
    gfxContext.SetDynamicSamplers( 0, _countof(Sampler), Sampler, { kBindPixel } );

    gfxContext.ClearColor( g_SceneColorBuffer );
    gfxContext.ClearDepth( g_SceneDepthBuffer );
    {
        ScopedTimer _prof( L"Render Color", gfxContext );

        gfxContext.SetViewportAndScissor( m_MainViewport, m_MainScissor );
        gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
        m_Mikudayo.DrawColor( gfxContext );
        Model::Flush( gfxContext );
        for (auto& primitive : m_Primitives)
            primitive->Draw( m_Camera.GetWorldSpaceFrustum() );
        Physics::Render( gfxContext, m_Camera.GetViewProjMatrix() );
    }
    gfxContext.SetRenderTarget( nullptr );
	gfxContext.Finish();
}

void Mikudayo::RenderUI( GraphicsContext & Context )
{
}
