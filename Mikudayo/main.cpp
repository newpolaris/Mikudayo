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
#include "MikuCamera.h"
#include "ShadowCamera.h"
#include "ShadowCameraUniform.h"
#include "ShadowCameraLiSPSM.h"
#include "CameraController.h"
#include "MikuCameraController.h"
#include "DebugHelper.h"
#include "ShadowCasterPass.h"
#include "RenderBonePass.h"
#include "SkinningPass.h"
#include "ForwardLighting.h"
#include "TaskManager.h"

// Effects
#include "PostEffects.h"
#include "TemporalEffects.h"
#include "Diffuse.h"

using namespace Math;
using namespace GameCore;
using namespace Graphics;
using namespace Math;

class Mikudayo : public GameCore::IGameApp
{
public:
	Mikudayo() : m_SkydomePass(kRenderQueueSkydome)
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
    ShadowCameraLiSPSM m_SunShadow;
    std::unique_ptr<CameraController> m_CameraController;
    std::unique_ptr<MikuCameraController> m_SecondCameraController;
	Motion m_Motion;

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

    RenderPass m_SkydomePass;
    SkinningPass m_RenderSkinPass;
    RenderBonePass m_RenderBonePass;
	ShadowCasterPass m_ShadowCasterPass;
};

CREATE_APPLICATION( Mikudayo )

enum { kCameraMain, kCameraVirtual, kCameraShadow };
const char* CameraNames[] = { "CameraMain", "CameraVirtual", "CameraShadow" };
EnumVar m_CameraType("Application/Camera/Camera Type", kCameraMain, 3, CameraNames );

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

    const Vector3 eye = Vector3(0.0f, 20.0f, 15.0f);
    m_Camera.SetEyeAtUp( eye, Vector3(0.0, 20.f, 0.f), Vector3(kYUnitVector) );
    m_Camera.SetPerspectiveMatrix( XM_PIDIV4, 9.0f/16.0f, 1.0f, 10000.0f );
    m_CameraController.reset(new CameraController(m_Camera, Vector3(kYUnitVector)));
    m_SecondCamera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
    m_SecondCameraController.reset(new MikuCameraController(m_SecondCamera, Vector3(kYUnitVector)));

    m_Scene = std::make_shared<Scene>();

    const std::wstring cameraMotion = L"Motion/クラブマジェスティカメラモーション.vmd";
    m_Motion.LoadMotion( cameraMotion );

    ModelInfo info;
    info.ModelFile = L"Model/つみ式ミクさんv1.1/ミクさん.pmx";
    info.MotionFile = L"Motion/クラブマジェスティ.vmd";
    SceneNodePtr instance = ModelManager::Load( info );
    if (instance) m_Scene->AddChild( instance );

    ModelInfo stage;
    stage.ModelFile = L"Stage/黒白チェスステージ/黒白チェスステージ.pmx";
    instance = ModelManager::Load( stage );
    if (instance) m_Scene->AddChild( instance );

    ModelInfo skydome;
    skydome.ModelFile = L"Stage/Skydome/incskies_030_8k.png";
    skydome.Type = kModelSkydome;
    instance = ModelManager::Load( skydome );
    if (instance) m_Scene->AddChild( instance );

    SceneNodePtr mirror = ModelManager::Load( L"Stage/Villa Fortuna Stage/MirrorWF/MirrorWF.pmx" );
    OrthogonalTransform rotation( Quaternion( -XM_PI/2, 0, 0 ) );
    if (mirror) {
        mirror->SetTransform( rotation );
        mirror->SetType( kSceneMirror );
        m_Scene->AddChild( mirror );
    }
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

const BaseCamera& Mikudayo::GetCamera()
{
    if (m_CameraType == kCameraVirtual)
        return m_Camera;
    else if (m_CameraType == kCameraShadow)
        return m_SunShadow;
    else
        return m_SecondCamera;
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
        // Update order is modified to hide physics update cost
        Physics::Wait();
        m_Scene->UpdateSceneAfterPhysics( m_Frame );
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

    m_Scene->Render( m_RenderSkinPass, args );
    D3D11_SAMPLER_HANDLE Sampler[] = { SamplerLinearWrap, SamplerLinearClamp, SamplerShadow, SamplerPointClamp };
    gfxContext.SetDynamicSamplers( 0, _countof(Sampler), Sampler, { kBindPixel } );
    {   
        ScopedTimer _prof( L"Render Shadow Map", gfxContext );
        m_SunShadow.UpdateMatrix( *m_Scene, m_SunDirection, m_Camera );
        gfxContext.SetDynamicConstantBufferView( 0, sizeof( m_SunShadow.GetViewProjMatrix() ), &m_SunShadow.GetViewProjMatrix(), { kBindVertex } );
        g_ShadowBuffer.BeginRendering( gfxContext );
        m_Scene->Render( m_ShadowCasterPass, args );
        g_ShadowBuffer.EndRendering( gfxContext );
    }
    {   
        ScopedTimer _prof( L"Render Color", gfxContext );
        gfxContext.ClearColor( g_SceneColorBuffer );
        gfxContext.ClearDepth( g_SceneDepthBuffer );
        gfxContext.ClearColor( g_EmissiveColorBuffer );
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
        vsConstants.cameraPosition = m_CameraPosition;
        vsConstants.viewToShadow = m_SunShadow.GetShadowMatrix();
        gfxContext.SetDynamicConstantBufferView( 0, sizeof( vsConstants ), &vsConstants, { kBindVertex } );
        gfxContext.SetDynamicDescriptor( 62, g_ShadowBuffer.GetSRV(), { kBindPixel } );

        Forward::Render( m_Scene, args );
        m_Scene->Render( m_SkydomePass, args );
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
	Context.SetViewportAndScissor( m_MainViewport, m_MainScissor );
}
