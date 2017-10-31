#include "stdafx.h"
#include "PrimitiveUtility.h"
#include "Bullet/Physics.h"
#include "Bullet/PhysicsPrimitive.h"
#include "Bullet/PrimitiveBatch.h"
#include "Bullet/LinearMath.h"
#include "ModelManager.h"
#include "RenderArgs.h"
#include "Scene.h"
#include "Motion.h"
#include "Camera.h"
#include "ShadowCamera.h"
#include "MikuCamera.h"
#include "CameraController.h"
#include "MikuCameraController.h"
#include "DebugHelper.h"
#include "ShadowCasterPass.h"
#include "RenderBonePass.h"
#include "ForwardLighting.h"
#include "TaskManager.h"
#include "TemporalEffects.h"

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
    const BaseCamera& GetCamera();

    Camera m_Camera;
    MikuCamera m_SecondCamera;
    ShadowCamera m_SunShadow;
    std::unique_ptr<CameraController> m_CameraController;
    std::unique_ptr<MikuCameraController> m_SecondCameraController;
	Motion m_Motion;

    const Vector3 m_MinBound = Vector3( -50, 0, -50 );
    const Vector3 m_MaxBound = Vector3( 50, 25, 50 );

    Vector3 m_SunColor;
    Vector3 m_SunDirection;
    Vector3 m_CameraPosition;

    Matrix4 m_ViewMatrix;
    Matrix4 m_ProjMatrix;
    Matrix4 m_ViewProjMatrix;
    D3D11_VIEWPORT m_MainViewport;
    D3D11_RECT m_MainScissor;

    btSoftBody* m_SoftBody;
    std::vector<Primitive::PhysicsPrimitivePtr> m_Primitives;
    std::shared_ptr<Scene> m_Scene;

    RenderBonePass m_RenderBonePass;
	ShadowCasterPass m_ShadowCasterPass;
};

CREATE_APPLICATION( Mikudayo )

enum { kCameraMain, kCameraVirtual, kCameraShadow };
const char* CameraNames[] = { "CameraMain", "CameraVirtual", "CameraShadow" };
EnumVar m_CameraType("Application/Camera/Camera Type", kCameraVirtual, 3, CameraNames );

NumVar m_Frame( "Application/Animation/Frame", 0, 0, 1e5, 1 );

// Default values in MMD. Due to RH coord, z is inverted.
NumVar m_SunDirX("Application/Lighting/Sun Dir X", -0.5f, -1.0f, 1.0f, 0.1f );
NumVar m_SunDirY("Application/Lighting/Sun Dir Y", -1.0f, -1.0f, 1.0f, 0.1f );
NumVar m_SunDirZ("Application/Lighting/Sun Dir Z", -0.5f, -1.0f, 1.0f, 0.1f );
NumVar m_SunColorR("Application/Lighting/Sun Color R", 157.f, 0.0f, 255.0f, 1.0f );
NumVar m_SunColorG("Application/Lighting/Sun Color G", 157.f, 0.0f, 255.0f, 1.0f );
NumVar m_SunColorB("Application/Lighting/Sun Color B", 157.f, 0.0f, 255.0f, 1.0f );

BoolVar s_bDrawBone( "Application/Model/Draw Bone", false );

void Mikudayo::Startup( void )
{
    TaskManager::Initialize();
    TextureManager::Initialize( L"Textures" );
    Physics::Initialize();
    PrimitiveUtility::Initialize();
    ModelManager::Initialize();
    Forward::Initialize();

    const Vector3 eye = Vector3(0.0f, 50.0f, 50.0f);
    m_Camera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
    m_Camera.SetPerspectiveMatrix( XM_PIDIV4, 9.0f/16.0f, 1.0f, 2000.0f );
    m_CameraController.reset(new CameraController(m_Camera, Vector3(kYUnitVector)));
    m_SecondCamera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
    m_SecondCameraController.reset(new MikuCameraController(m_SecondCamera, Vector3(kYUnitVector)));

    // TemporalEffects::EnableTAA = true;
    // PostEffects::EnableHDR = true;

    g_SceneColorBuffer.SetClearColor( Color(1.f, 1.f, 1.f, 1.f).FromSRGB() );

    std::vector<Primitive::PhysicsPrimitiveInfo> primitves = {
    #if 0
        { kPlaneShape, 0.f, Vector3( kZero ), Vector3( 0, -1, 0 ) },
        { kBoxShape, 20.f, Vector3( 10, 1, 10 ), Vector3( 0, 2, 0 ) },
        { kBoxShape, 20.f, Vector3( 2,1,5 ), Vector3( 10, 2, 0 ) },
        { kBoxShape, 20.f, Vector3( 8,1,2 ), Vector3( 0, 2, 10 ) },
        { kBoxShape, 20.f, Vector3( 8,1,2 ), Vector3( 0, 2, -13 ) },
    #endif
    };
    for (auto& info : primitves)
        m_Primitives.push_back( std::move( Primitive::CreatePhysicsPrimitive( info ) ) );

    m_Scene = std::make_shared<Scene>();

    SceneNodePtr instance;
#if 0
    const std::wstring testModel = L"Model/PDF 2nd Freely Tomorrow Stage/Freely Tomorrow Stage.x";
    // const std::wstring testModel = L"Model/vikings_islands/Islands.obj";
    instance = ModelManager::Load( testModel );
    m_Scene->AddChild( instance );
#endif

// #define HALLOWEEN 1
// #define BOARD 1
#define FLOOR 1
// #define STAGE 1

    ModelInfo stage;
#if BOARD
    stage.ModelFile = L"Model/黒白チェスステージ/黒白チェスステージ.pmx";
#elif HALLOWEEN
    stage.ModelFile = L"Model/HalloweenStage/halloween.Pmx";
#elif FLOOR
    stage.ModelFile = L"Model/Floor.pmx";
#elif STAGE
    instance = ModelManager::Load( L"Model/Villa Fortuna Stage/screens.x" );
    if (instance)
        m_Scene->AddChild( instance );
    stage.ModelFile = L"Model/Villa Fortuna Stage/villa_fontana.pmx";
#endif
    instance = ModelManager::Load( stage );
    if (instance)
        m_Scene->AddChild( instance );

    // const std::wstring motion = L"Motion/nekomimi_lat.vmd";
    // const std::wstring motion = L"Motion/クラブマジェスティ.vmd";
    const std::wstring motion = L"";
    const std::wstring cameraMotion = L"Motion/クラブマジェスティカメラモーション.vmd";

    m_Motion.LoadMotion( cameraMotion );

    ModelInfo info;
    info.ModelFile = L"Model/Tda/Tda式初音ミク・アペンド_Ver1.10.pmx";
    info.ModelFile = L"Model/Tda式デフォ服ミク_ver1.1/Tda式初音ミク_デフォ服ver.pmx";
    // info.ModelFile = L"Model/on_SHIMAKAZE_v090/onda_mod_SHIMAKAZE_v091.pmx";
    // info.ModelFile = L"Model/Tda式改変ミク　JKStyle/Tda式改変ミク　JKStyle.pmx";
    // info.ModelFile = L"Model/Tda式初音ミク背中見せデフォ服 Ver1.00/Tda式初音ミク背中見せデフォ服 ver1.0无高光.pmx";
    info.MotionFile = motion;

    instance = ModelManager::Load( info );
    if (instance)
        m_Scene->AddChild( instance );

#if 0
    SceneNodePtr mirror = ModelManager::Load( L"Model/Villa Fortuna Stage/MirrorWF/MirrorWF.pmx" );
    OrthogonalTransform rotation( Quaternion( -3.14/2, 0, 0 ) );
    mirror->SetTransform( rotation );
    mirror->SetType( kSceneMirror );
    m_Scene->AddChild( mirror );
#endif
}

void Mikudayo::Cleanup( void )
{
    Physics::Stop();
    m_Scene.reset();
    ModelManager::Shutdown();
    PrimitiveUtility::Shutdown();
    for (auto& model : m_Primitives)
        model->Destroy();
    m_Primitives.clear();
    Forward::Shutdown();
    Physics::Shutdown();
    TaskManager::Shutdown();
}

void Mikudayo::Update( float deltaT )
{
    ScopedTimer _prof( L"Update" );

    if (CameraMove == kCameraMoveMotion)
        m_Motion.Animate( m_SecondCamera );

    if (m_CameraType == kCameraVirtual)
        m_CameraController->Update( deltaT );
    else if (m_CameraType == kCameraMain)
        m_SecondCameraController->Update( deltaT );

    m_SunDirection = Vector3( m_SunDirX, m_SunDirY, m_SunDirZ );
    m_SunColor = Vector3( m_SunColorR, m_SunColorG, m_SunColorB );

    m_CameraPosition = GetCamera().GetPosition();
    m_ViewMatrix = GetCamera().GetViewMatrix();
    m_ProjMatrix = GetCamera().GetProjMatrix();
    m_ViewProjMatrix = GetCamera().GetViewProjMatrix();

    // We use viewport offsets to jitter sample positions from frame to frame (for TAA.)
    // D3D has a design quirk with fractional offsets such that the implicit scissor
    // region of a viewport is floor(TopLeftXY) and floor(TopLeftXY + WidthHeight), so
    // having a negative fractional top left, e.g. (-0.25, -0.25) would also shift the
    // BottomRight corner up by a whole integer.  One solution is to pad your viewport
    // dimensions with an extra pixel.  My solution is to only use positive fractional offsets,
    // but that means that the average sample position is +0.5, which I use when I disable
    // temporal AA.
    TemporalEffects::GetJitterOffset(m_MainViewport.TopLeftX, m_MainViewport.TopLeftY);

	m_MainViewport.Width = (float)g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height = (float)g_SceneColorBuffer.GetHeight();
	m_MainViewport.MinDepth = 0.0f;
	m_MainViewport.MaxDepth = 1.0f;

	m_MainScissor.left = 0;
	m_MainScissor.top = 0;
	m_MainScissor.right = (LONG)g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom = (LONG)g_SceneColorBuffer.GetHeight();

    if (!EngineProfiling::IsPaused())
        m_Frame = m_Frame + deltaT * 30.f;
    {
        // TODO: Try lock (delay physics update - motion only)
        Physics::Wait();
        m_Scene->UpdateSceneAfterPhysics( m_Frame );
        // TODO: Move draw call and fix frame step
        m_Scene->UpdateScene( m_Frame );
        Physics::Update( deltaT );
        m_Motion.Update( m_Frame );
    }
    for (auto& primitive : m_Primitives)
        primitive->Update();
    Physics::UpdatePicking( m_MainViewport, GetCamera() );
}

void Mikudayo::RenderScene( void )
{
	GraphicsContext& gfxContext = GraphicsContext::Begin( L"Scene Render" );
    RenderArgs args = { gfxContext, m_ViewMatrix, m_ProjMatrix, m_MainViewport, GetCamera() };

    __declspec(align(16)) struct
    {
        Vector3 LightDirection;
        Vector3 LightColor;
        float ShadowTexelSize[4];
    } psConstants;
    psConstants.LightDirection = m_SunDirection;
    psConstants.LightColor = m_SunColor / Vector3( 255.f, 255.f, 255.f );
    psConstants.ShadowTexelSize[0] = 1.0f / g_ShadowBuffer.GetWidth();
	gfxContext.SetDynamicConstantBufferView( 5, sizeof(psConstants), &psConstants, { kBindVertex, kBindPixel } );

    D3D11_SAMPLER_HANDLE Sampler[] = { SamplerLinearWrap, SamplerLinearClamp, SamplerShadow };
    gfxContext.SetDynamicSamplers( 0, _countof(Sampler), Sampler, { kBindPixel } );
    {
        ScopedTimer _prof(L"Render Shadow Map", gfxContext);
        float Radius = Length( m_MaxBound - m_MinBound ) / Scalar(2);
        Vector3 SunPosition = -m_SunDirection * Radius;
        m_SunShadow.UpdateMatrix( m_SunDirection, SunPosition, Scalar(Radius*2), (uint32_t)g_ShadowBuffer.GetWidth(), (uint32_t)g_ShadowBuffer.GetHeight(), 16);
        // m_SunShadow.UpdateMatrix( m_SunDirection, SunPosition, Scalar( Radius * 2 ), m_Camera );

        gfxContext.SetDynamicConstantBufferView( 0, sizeof( m_SunShadow.GetViewProjMatrix() ), &m_SunShadow.GetViewProjMatrix(), { kBindVertex } );
        g_ShadowBuffer.BeginRendering( gfxContext );
        m_Scene->Render( m_ShadowCasterPass, args );
        g_ShadowBuffer.EndRendering( gfxContext );
    }
    {
        gfxContext.ClearColor( g_SceneColorBuffer );
        gfxContext.ClearDepth( g_SceneDepthBuffer );
        gfxContext.SetViewportAndScissor( m_MainViewport, m_MainScissor );

        struct VSConstants
        {
            Matrix4 view;
            Matrix4 projection;
            Matrix4 viewToShadow;
            Vector3 cameraPosition;
        } vsConstants;
        vsConstants.view = m_ViewMatrix;
        vsConstants.projection = m_ProjMatrix;
        vsConstants.viewToShadow = m_SunShadow.GetShadowMatrix();
        vsConstants.cameraPosition = m_CameraPosition;
        gfxContext.SetDynamicConstantBufferView( 0, sizeof( vsConstants ), &vsConstants, { kBindVertex } );
        gfxContext.SetDynamicDescriptor( 62, g_ShadowBuffer.GetSRV(), { kBindPixel } );

        ScopedTimer _prof( L"Render Color", gfxContext );
        Forward::Render( m_Scene, args );
    }
    {
        ScopedTimer _prof( L"Primitive Color", gfxContext );
        PrimitiveUtility::Flush( gfxContext );
        for (auto& primitive : m_Primitives)
            primitive->Draw( GetCamera().GetWorldSpaceFrustum() );
        Physics::Render( gfxContext, GetCamera().GetViewProjMatrix() );
    }
    gfxContext.SetRenderTarget( nullptr );

    TemporalEffects::ResolveImage(gfxContext);

	gfxContext.Finish();
}

void Mikudayo::RenderUI( GraphicsContext& Context )
{
    RenderArgs args = { Context, m_ViewMatrix, m_ProjMatrix, m_MainViewport, GetCamera() };

    if (s_bDrawBone)
        m_Scene->Render( m_RenderBonePass, args );
    Physics::RenderDebug( Context, GetCamera().GetViewProjMatrix() );
    // Utility::DebugTexture( Context, g_ShadowBuffer.GetSRV() );
    // Utility::DebugTexture( Context, g_aBloomUAV1[0].GetSRV() );
    // Utility::DebugTexture( Context, g_ReflectColorBuffer.GetSRV() );
	Context.SetViewportAndScissor( m_MainViewport, m_MainScissor );
}

const BaseCamera& Mikudayo::GetCamera()
{
    if (m_CameraType == kCameraVirtual)
        return m_Camera;
    else if (m_CameraType == kCameraShadow)
        return m_SunShadow;
    else
        return m_SecondCamera;
}