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
    const Camera& GetCamera();

    Camera m_Camera, m_SecondCamera;
    std::auto_ptr<CameraController> m_CameraController, m_SecondCameraController;

    Vector3 m_SunColor;
    Vector3 m_SunDirection;

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

enum { kCameraMain, kCameraVirtual };
const char* CameraNames[] = { "CameraMain", "CameraVirtual" };
EnumVar m_CameraType("Application/Camera/Camera Type", kCameraMain, kCameraVirtual+1, CameraNames );

// Default values in MMD. Due to RH coord, z is inverted.
NumVar m_SunDirX("Application/Lighting/Sun Dir X", -0.5f, -1.0f, 1.0f, 0.1f );
NumVar m_SunDirY("Application/Lighting/Sun Dir Y", -1.0f, -1.0f, 1.0f, 0.1f );
NumVar m_SunDirZ("Application/Lighting/Sun Dir Z", -0.5f, -1.0f, 1.0f, 0.1f );
NumVar m_SunColorR("Application/Lighting/Sun Color R", 157.f, 0.0f, 255.0f, 1.0f );
NumVar m_SunColorG("Application/Lighting/Sun Color G", 157.f, 0.0f, 255.0f, 1.0f );
NumVar m_SunColorB("Application/Lighting/Sun Color B", 157.f, 0.0f, 255.0f, 1.0f );

void Mikudayo::Startup( void )
{
    TextureManager::Initialize( L"Textures" );
    Physics::Initialize();
    Model::Initialize();

    const Vector3 eye = Vector3(0.0f, 18.0f, -15.0f);
    m_Camera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
    m_CameraController.reset(new CameraController(m_Camera, Vector3(kYUnitVector)));
    m_SecondCamera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
    m_SecondCameraController.reset(new CameraController(m_SecondCamera, Vector3(kYUnitVector)));

    Rendering::PmxModel mikudayo;
    mikudayo.LoadFromFile( L"Model/Mikudayo/mikudayo-3_6.pmx" );
    m_Mikudayo = std::move(mikudayo);

    auto vertices = m_Mikudayo.GetVertices();
    auto indices = m_Mikudayo.GetIndices();
#if 0
    SoftBodyGeometry Geo { vertices, indices };
    m_SoftBody = manager.LoadFromGeometry( Geo );
    m_SoftBody->translate( btVector3( 0, 2, 0 ) );
#endif

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

    if (m_CameraType == kCameraMain)
        m_CameraController->Update( deltaT );
    else
        m_SecondCameraController->Update( deltaT );

    m_SunDirection = Vector3( m_SunDirX, m_SunDirY, m_SunDirZ );
    m_SunColor = Vector3( m_SunColorR, m_SunColorG, m_SunColorB );

    m_ViewMatrix = GetCamera().GetViewMatrix();
    m_ProjMatrix = GetCamera().GetProjMatrix();
    m_ViewProjMatrix = GetCamera().GetViewProjMatrix();

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

    auto GetRayTo = [&]( float x, float y) {
        auto& invView = m_Camera.GetCameraToWorld();
        auto& proj = m_Camera.GetProjMatrix();
        auto p00 = proj.GetX().GetX();
        auto p11 = proj.GetY().GetY();
        float vx = (+2.f * x / m_MainViewport.Width - 1.f) / p00;
        float vy = (-2.f * y / m_MainViewport.Height + 1.f) / p11;
        Vector3 Dir(XMVector3TransformNormal( Vector3( vx, vy, -1 ), Matrix4(invView) ));
        return Normalize(Dir);
    };

    using namespace GameInput;
    static bool bMouseDrag = false;
    if (IsFirstReleased( DigitalInput::kMouse0 ))
    {
        bMouseDrag = false;
        Physics::ReleasePickBody();
    }
    if (IsFirstPressed( DigitalInput::kMouse0 ) || bMouseDrag)
    {
        float sx = (float)GetMousePosition( 0 ), sy = (float)GetMousePosition( 1 );
        Vector3 rayFrom = m_Camera.GetPosition();
        Vector3 rayTo = GetRayTo( sx, sy ) * m_Camera.GetFarClip() + rayFrom;
        if (!bMouseDrag)
        {
            Physics::PickBody( Convert( rayFrom ), Convert( rayTo ), Convert( m_Camera.GetForwardVec() ) );
            bMouseDrag = true;
        }
        else
        {
            Physics::MovePickBody( Convert( rayFrom ), Convert( rayTo ), Convert( m_Camera.GetForwardVec() ) );
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
    vsConstants.view = m_ViewMatrix;
    vsConstants.projection = m_ProjMatrix;
	gfxContext.SetDynamicConstantBufferView( 0, sizeof(vsConstants), &vsConstants, { kBindVertex } );

    __declspec(align(16)) struct
    {
        Vector3 LightDirection;
        Vector3 LightColor;
    } psConstants;

    psConstants.LightDirection = m_Camera.GetViewMatrix().Get3x3() * m_SunDirection;
    psConstants.LightColor = m_SunColor / Vector3( 255.f, 255.f, 255.f );
	gfxContext.SetDynamicConstantBufferView( 1, sizeof(psConstants), &psConstants, { kBindPixel } );

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
            primitive->Draw( GetCamera().GetWorldSpaceFrustum() );
        Physics::Render( gfxContext, GetCamera().GetViewProjMatrix() );
    }
    gfxContext.SetRenderTarget( nullptr );
	gfxContext.Finish();
}

void Mikudayo::RenderUI( GraphicsContext & Context )
{
}

const Camera& Mikudayo::GetCamera()
{
    if (m_CameraType == kCameraVirtual)
        return m_SecondCamera;
    else
        return m_Camera;
}