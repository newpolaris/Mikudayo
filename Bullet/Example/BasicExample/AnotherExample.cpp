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
#include "PrimitiveBatch.h"
#include "ParallelFor.h"
#include <ppl.h>

using namespace GameCore;
using namespace Graphics;
using namespace Math;

class BasicExample : public GameCore::IGameApp
{
public:
	BasicExample()
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

CREATE_APPLICATION( BasicExample )

namespace {
    static int uRigidNum = 0;
}

void BasicExample::Startup( void )
{
    TextureManager::Initialize( L"Textures" );
    Physics::Initialize();
    PrimitiveBatch::Initialize();

    const Vector3 eye = Vector3(62.0f, 18.0f, -15.0f);
    m_Camera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
    m_CameraController.reset(new CameraController(m_Camera, Vector3(kYUnitVector)));

    std::vector<Primitive::PhysicsPrimitiveInfo> Info = {
        { Physics::kPlaneShape, 0.f, Vector3( kZero ), Vector3( kZero ) },
        { Physics::kBoxShape, 20.f, Vector3( 2,1,5 ), Vector3( -10, 2, 0 ) },
        { Physics::kBoxShape, 20.f, Vector3( 2,1,5 ), Vector3( 10, 2, 0 ) },
        { Physics::kBoxShape, 20.f, Vector3( 8,1,2 ), Vector3( 0, 2, 10 ) },
        { Physics::kBoxShape, 20.f, Vector3( 8,1,2 ), Vector3( 0, 2, -13 ) },
    };

    {
        const int X = 7, Y = 20, Z = 6;
        const float GAP = 0.2f;
        const float box = 0.6f;
        const float mass = 1.0f;
        for (int y = 0; y < Y; ++y) {
            float vy = (box * 3 + GAP)*y;
            for (int x = 0; x < X; ++x) {
                float vx = (box * 3 + GAP)*(x - X / 2) + 0.0f*y;
                for (int z = 0; z < Z; ++z) {
                    float vz = (box * 3 + GAP)*(z - Z / 2);
                    Info.push_back( {Physics::ShapeType(rand() % 5), mass, Vector3( box, box, box ), Vector3( vx, vy + 15, vz )});
                }
            }
        }
    }
    {
        const int X = 8, Y = 36, Z = 8;
        const float GAP = 0.05f;
        const float box = 1.0f;
        const float mass = 1.0f;
        for (int y = 0; y < Y; ++y) {
            float vy = (box * 2 + GAP)*y;
            for (int x = 0; x < X; ++x) {
                float vx = (box * 2 + GAP)*(x - X / 2) + 0.3f*y;
                for (int z = 0; z < Z; ++z) {
                    float vz = (box * 2 + GAP)*(z - Z / 2);
                    Primitive::PhysicsPrimitiveInfo cinfo = {
                        Physics::kBoxShape, mass, Vector3( box,box,box ), Vector3( vx,vy + 1,vz + 50 )
                    };
                    if ((x + z + y / 2) % 2) {
                        m_Models.push_back( std::move( Primitive::CreatePhysicsPrimitive( cinfo ) ) );
                    }
                }
            }
        }
    }
    for (auto& info : Info)
        m_Models.push_back( std::move( Primitive::CreatePhysicsPrimitive( info ) ) );
}

void BasicExample::Cleanup( void )
{
    for (auto& model : m_Models)
        model->Destroy();
    m_Models.clear();
    PrimitiveBatch::Shutdown();
    Physics::Shutdown();
}

template <typename Func>
void parallelFor( size_t Begin, size_t End, Func func)
{
    concurrency::parallel_for( Begin, End, func );
}

void BasicExample::Update( float deltaT )
{
    ScopedTimer _prof( L"Update" );
    if (!EngineProfiling::IsPaused())
    {
        Physics::Update( deltaT );
        {
            ScopedTimer _( L"Update2" );
            parallelFor( size_t(0), m_Models.size(), [&](size_t i) {
                m_Models[i]->UpdateTransform();
            } );
        }
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

void BasicExample::RenderScene( void )
{
    struct {
        Matrix4 ViewToClip;
    } vsConstants { m_ProjMatrix*m_ViewMatrix };
    struct {
        Vector3 CameraPosition;
    } psConstants { m_Camera.GetPosition() };

    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");

    gfxContext.SetViewportAndScissor( m_MainViewport, m_MainScissor );
    gfxContext.ClearColor( g_SceneColorBuffer );
    gfxContext.ClearDepth( g_SceneDepthBuffer );
    gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
    gfxContext.SetDynamicSampler( 0, SamplerLinearWrap, { kBindPixel } );
    gfxContext.SetDynamicConstantBufferView( 0, sizeof(vsConstants), &vsConstants, { kBindVertex } );
    gfxContext.SetDynamicConstantBufferView( 0, sizeof(psConstants), &psConstants, { kBindPixel } );
    {
        ScopedTimer _prof( L"Primitive Draw", gfxContext );
        for (auto& model : m_Models)
            model->Draw(m_Camera.GetWorldSpaceFrustum());
        PrimitiveBatch::Flush( gfxContext );
    }
    gfxContext.Finish();
}

void BasicExample::RenderUI( GraphicsContext& Context )
{
    Physics::Render( Context, m_ViewProjMatrix );
	int32_t x = g_OverlayBuffer.GetWidth() - 500;

    Physics::ProfileStatus Status;
    Physics::Profile( Status );

	TextContext UiContext(Context);
	UiContext.Begin();
    UiContext.ResetCursor( float(x), 10.f );
    UiContext.SetColor(Color( 0.7f, 1.0f, 0.7f ));

#define DebugAttribute(Attribute) \
    UiContext.DrawFormattedString( ""#Attribute " %3d", Status.Attribute ); \
    UiContext.NewLine();

    DebugAttribute(NumIslands);
    DebugAttribute(NumCollisionObjects);
    DebugAttribute(NumManifolds);
    DebugAttribute(NumContacts);
    DebugAttribute(NumThread);
#undef DebugAttribute

#define DebugAttribute(Attribute) \
    UiContext.DrawFormattedString( ""#Attribute " %5.3f", Status.Attribute ); \
    UiContext.NewLine();

    DebugAttribute(InternalTimeStep);
    DebugAttribute(DispatchAllCollisionPairs);
    DebugAttribute(DispatchIslands);
    DebugAttribute(PredictUnconstrainedMotion);
    DebugAttribute(CreatePredictiveContacts);
    DebugAttribute(IntegrateTransforms);
#undef DebugAttribute
}
