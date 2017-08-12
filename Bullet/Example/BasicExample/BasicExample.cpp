#include <iostream>

#include "GameCore.h"
#include "GraphicsCore.h"
#include "PipelineState.h"
#include "CommandContext.h"
#include "GpuBuffer.h"
#include "InputLayout.h"
#include "ColorBuffer.h"
#include "BufferManager.h"
#include "Camera.h"
#include "CameraController.h"
#include "SamplerManager.h"
#include "GameInput.h"
#include "Physics.h"
#include "TextureManager.h"
#include "PhysicsPrimitive.h"

using namespace GameCore;
using namespace Graphics;
using namespace Math;

class ModelViewer : public GameCore::IGameApp
{
public:
	ModelViewer()
	{
	}

	virtual void Startup( void ) override;
	virtual void Cleanup( void ) override;

	virtual void Update( float deltaT ) override;
	virtual void RenderScene( void ) override;
    virtual void RenderUI( GraphicsContext & Context ) override;

private:

    Camera m_Camera;
    std::auto_ptr<CameraController> m_CameraController;

    Matrix4 m_ViewMatrix;
    Matrix4 m_ProjMatrix;
    Matrix4 m_ViewProjMatrix;
    D3D11_VIEWPORT m_MainViewport;
    D3D11_RECT m_MainScissor;

    GraphicsPSO m_DepthPSO;
    GraphicsPSO m_CutoutDepthPSO;
    GraphicsPSO m_ModelPSO;

    std::vector<Primitive::PhysicsPrimitivePtr> m_Models;
};

CREATE_APPLICATION( ModelViewer )

namespace {
    static int uRigidNum = 0;
}

void ModelViewer::Startup( void )
{
    TextureManager::Initialize( L"Textures" );
    Physics::Initialize();

    const Vector3 eye = Vector3(0.0f, 10.0f, 10.0f);
    m_Camera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
    m_CameraController.reset(new CameraController(m_Camera, Vector3(kYUnitVector)));

	{
        Primitive::PhysicsPrimitiveInfo Info[] = {
#if 1
            { Physics::kPlaneShape, 0.f, Vector3( kZero ), Vector3( kZero ) },
#else
            { Physics::kBoxShape, 0.f, Scalar( 50.f ), Vector3( 0, -50, 0 ) },
#endif
            { Physics::kBoxShape, 20.f, Vector3( 2,1,5 ), Vector3( -10, 2, 0 ) },
            { Physics::kBoxShape, 20.f, Vector3( 2,1,5 ), Vector3( 10, 2, 0 ) },
            { Physics::kBoxShape, 20.f, Vector3( 8,1,2 ), Vector3( 0, 2, 10 ) },
            { Physics::kBoxShape, 20.f, Vector3( 8,1,2 ), Vector3( 0, 2, -13 ) },
        };
        for (auto& info : Info)
            m_Models.push_back( std::move( Primitive::CreatePhysicsPrimitive( info ) ) );
    }
}

void ModelViewer::Cleanup( void )
{
    for (auto& model : m_Models)
        model->Destroy();
    m_Models.clear();
    Physics::Shutdown();
}

void ModelViewer::Update( float deltaT )
{
    ScopedTimer _prof( L"Bullet Update" );

    if (!EngineProfiling::IsPaused())
    {
        static int uCount = 0;
        if ((uCount++ % 2) == 0 && uRigidNum++ < 600 ) {
            static UINT rigid = 0;
            auto randf = []() { return (float)rand() / (float)RAND_MAX; };
            auto randrf = [randf]( float mn, float mx ) { return randf()*(mx - mn) + mn; };
            FLOAT x = randrf( -5, 5 );
            FLOAT y = 15.0f + randf()*2.0f;
            FLOAT z = randrf( -5, 5 );
            FLOAT sx = randrf( 0.5f, 1.0f );
            FLOAT sy = randrf( 0.5f, 1.0f );
            FLOAT sz = randrf( 0.5f, 1.0f );
            FLOAT mass = randrf( 0.8f, 1.2f );

            m_Models.push_back( std::move( Primitive::CreatePhysicsPrimitive(
                { Physics::ShapeType(uRigidNum % 5), mass, Vector3( sx,sy,sz ), Vector3( x,y,z )  }
            ) ) );
        }
        Physics::Update( deltaT );
    }

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
}

void ModelViewer::RenderScene( void )
{
    struct {
        Matrix4 View;
        Matrix4 Proj;
    } vsConstants {
        m_ViewMatrix,
        m_ProjMatrix
    };

    struct {
        Vector3 CameraPosition;
    } psConstants {
        m_Camera.GetPosition()
    };

    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");

    gfxContext.SetViewportAndScissor( m_MainViewport, m_MainScissor );
    gfxContext.ClearColor( g_SceneColorBuffer );
    gfxContext.ClearDepth( g_SceneDepthBuffer );
    gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
    gfxContext.SetDynamicSampler( 0, SamplerLinearWrap, { kBindPixel } );
    gfxContext.SetDynamicConstantBufferView( 0, sizeof(vsConstants), &vsConstants, { kBindVertex } );
    gfxContext.SetDynamicConstantBufferView( 0, sizeof(psConstants), &psConstants, { kBindPixel } );
    for (auto& model : m_Models)
        model->Draw( gfxContext );
    gfxContext.Finish();
}

void ModelViewer::RenderUI( GraphicsContext& Context )
{
    Physics::Render( Context, m_ViewProjMatrix );
	int32_t x = g_OverlayBuffer.GetWidth() - 300;

	TextContext UiContext(Context);
	UiContext.Begin();
    UiContext.ResetCursor( float(x), 10.f );
    UiContext.SetColor(Color( 0.7f, 1.0f, 0.7f ));
    UiContext.DrawFormattedString( "Primitive Count %3d", uRigidNum );
}
