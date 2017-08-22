#include "GameCore.h"
#include "GraphicsCore.h"
#include "PipelineState.h"
#include "CommandContext.h"
#include "GpuBuffer.h"
#include "InputLayout.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "ShadowBuffer.h"
#include "ConstantBuffer.h"
#include "TemporalEffects.h"
#include "TextureManager.h"
#include "BufferManager.h"
#include "MotionBlur.h"
#include "DepthOfField.h"
#include "ShadowCamera.h"
#include "MikuCamera.h"
#include "MikuCameraController.h"
#include "CameraController.h"
#include "GameInput.h"
#include "Motion.h"
#include "FXAA.h"
#include "DebugHelper.h"
#include "GroundPlane.h"
#include "Shadow.h"
#include "ModelLoader.h"
#include "ModelBase.h"
#include "Math/BoundingBox.h"
#include "OrthographicCamera.h"

#include "CompiledShaders/PmdOpaqueVS.h"
#include "CompiledShaders/PmdOpaquePS.h"
#include "CompiledShaders/PmxOpaqueVS.h"
#include "CompiledShaders/PmxOpaquePS.h"
#include "CompiledShaders/GroundOpaqueVS.h"
#include "CompiledShaders/GroundOpaquePS.h"
#include "CompiledShaders/PmdDepthViewerVS.h"
#include "CompiledShaders/PmxDepthViewerVS.h"
#include "CompiledShaders/DepthViewerPS.h"

using namespace DirectX;
using namespace GameCore;
using namespace Graphics;
using namespace Math;

namespace Pmx {
	std::vector<InputDesc> InputDescriptor
	{
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_ID", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "EDGE_FLAT", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
}
namespace Pmd {
	std::vector<InputDesc> InputDescriptor
	{
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_ID", 0, DXGI_FORMAT_R16G16_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHT", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "EDGE_FLAT", 0, DXGI_FORMAT_R8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
}

enum { kShadowSplit = 4 };

namespace Lighting
{
    enum { kMaxLights = 4 };
    enum { kShadowDim = 512 };
    ColorBuffer m_LightShadowArray;
    ShadowBuffer m_LightShadowTempBuffer;

    void Initialize();
    void Shutdown();
}

void Lighting::Initialize( void )
{
    m_LightShadowArray.CreateArray(L"m_LightShadowArray", kShadowDim, kShadowDim, kMaxLights, DXGI_FORMAT_R16_UNORM);
    m_LightShadowTempBuffer.Create(L"m_LightShadowTempBuffer", kShadowDim, kShadowDim);
}

void Lighting::Shutdown( void )
{
    m_LightShadowArray.Destroy();
    m_LightShadowTempBuffer.Destroy();
}

class MikuViewer : public GameCore::IGameApp
{
public:
    MikuViewer();

	virtual void Startup( void ) override;
	virtual void Cleanup( void ) override;

	virtual void Update( float deltaT ) override;
	virtual void RenderScene( void ) override;

    void RenderUI( GraphicsContext& Context ) override;

private:

    BoundingBox GetBoundingBox();
    void RenderObjects( GraphicsContext& gfxContext, const Matrix4& ViewProjMat, eObjectFilter Filter );
    void RenderObjects( GraphicsContext& gfxContext, const Matrix4 & ViewMat, const Matrix4 & ProjMat, eObjectFilter Filter );
    void RenderLightShadows(GraphicsContext& gfxContext);
    void RenderShadowMap(GraphicsContext& gfxContext);
    MikuCamera* SelectedCamera();

	MikuCamera m_Camera;
    MikuCamera m_SecondCamera;
	MikuCameraController* m_pCameraController;
	CameraController* m_pSecondCameraController;

    Matrix4 m_ViewMatrix;
    Matrix4 m_ProjMatrix;

	D3D11_VIEWPORT m_MainViewport;
	float m_JitterDelta[2];
	D3D11_RECT m_MainScissor;

    Vector3 m_SunDirection;
    Vector3 m_SunColor;
    ShadowCamera m_SunShadow;

    using IRenderObjectPtr = std::shared_ptr<Graphics::IRenderObject>;
    std::vector<IRenderObjectPtr> m_Models;
	Graphics::Motion m_Motion;

	GraphicsPSO m_DepthPSO[kModelMAX];
    GraphicsPSO m_ShadowPSO[kModelMAX];
	GraphicsPSO m_OpaquePSO[kModelMAX];
	GraphicsPSO m_BlendPSO[kModelMAX];

    std::vector<Matrix4> m_ShadowViewProj;
    std::vector<FrustumCorner> m_SplitFrustum;
    std::vector<FrustumCorner> m_ViewFrustum;
};

CREATE_APPLICATION( MikuViewer )

NumVar m_Frame( "Application/Animation/Frame", 0, 0, 1e5, 1 );

enum { kCameraMain, kCameraVirtual };
const char* CameraNames[] = { "CameraMain", "CameraVirtual" };
EnumVar m_CameraType("Application/Camera/Camera Type", kCameraMain, kCameraVirtual+1, CameraNames );
BoolVar m_bDebugTexture("Application/Camera/Debug Texture", false);
BoolVar m_bViewSplit("Application/Camera/View Split", false);
BoolVar m_bShadowSplit("Application/Camera/Shadow Split", false);
BoolVar m_bFixDepth("Application/Camera/Fix Depth", false);
BoolVar m_bLightFrustum("Application/Camera/Light Frustum", false);
BoolVar m_bStabilizeCascades("Application/Camera/Stabilize Cascades", false);

ExpVar m_SunLightIntensity("Application/Lighting/Sun Light Intensity", 4.0f, 0.0f, 16.0f, 0.1f);
ExpVar m_AmbientIntensity("Application/Lighting/Ambient Intensity", 0.1f, -16.0f, 16.0f, 0.1f);
NumVar m_ShadowDimX("Application/Lighting/Shadow Dim X", 100, 10, 1000, 10 );
NumVar m_ShadowDimY("Application/Lighting/Shadow Dim Y", 100, 10, 1000, 10 );
NumVar m_ShadowDimZ("Application/Lighting/Shadow Dim Z", 100, 10, 1000, 10 );

// Default values in MMD. Due to RH coord, z is inverted.
NumVar m_SunDirX("Application/Lighting/Sun Dir X", -0.5f, -1.0f, 1.0f, 0.1f );
NumVar m_SunDirY("Application/Lighting/Sun Dir Y", -1.0f, -1.0f, 1.0f, 0.1f );
NumVar m_SunDirZ("Application/Lighting/Sun Dir Z", -0.5f, -1.0f, 1.0f, 0.1f );
NumVar m_SunColorR("Application/Lighting/Sun Color R", 157.f, 0.0f, 255.0f, 1.0f );
NumVar m_SunColorG("Application/Lighting/Sun Color G", 157.f, 0.0f, 255.0f, 1.0f );
NumVar m_SunColorB("Application/Lighting/Sun Color B", 157.f, 0.0f, 255.0f, 1.0f );

BoolVar ShowWaveTileCounts("Application/Forward+/Show Wave Tile Counts", false);
#ifdef _WAVE_OP
BoolVar EnableWaveOps("Application/Forward+/Enable Wave Ops", true);
#endif

MikuViewer::MikuViewer() : m_pCameraController( nullptr ), m_pSecondCameraController( nullptr )
{
    float Sign = Math::g_ReverseZ ? -1.f : 1.f;
    for (auto Desc : {&RasterizerShadow, &RasterizerShadowCW, &RasterizerShadowTwoSided})
    {
        Desc->SlopeScaledDepthBias = Sign * 2.0f;
        Desc->DepthBias = static_cast<INT>(Sign * 0);
    }
}

void MikuViewer::Startup( void )
{
	TextureManager::Initialize( L"Textures" );
    Lighting::Initialize();
    ModelBase::Initialize();

    struct ModelInit
    {
        std::wstring Model;
        std::wstring Motion;
        XMFLOAT3 Position;
    };

    ModelLoader Loader( L"Models/mikudayo-3_6_.pmx", L"", XMFLOAT3( 15, 0, 0 ) );
    auto model = Loader.Load();
    if (model)
        m_Models.push_back( model );

    auto motionPath = L"Motions/nekomimi_lat.vmd";
    std::vector<ModelInit> list = {
        // { L"Models/mikudayo-3_6_.pmx", L"", XMFLOAT3( 0, 0, 0 ) },
        // { L"Models/観客_右利き_サイリウム有AL.pmx", L"", XMFLOAT3( 0, 0, 0 ) },
        { L"Models/Tda式初音ミク_デフォ服ver.pmx", L"", XMFLOAT3( 15, 0, 0 ) },
#if 0
        { L"Models/Lat0.pmd", motionPath, XMFLOAT3( -10.f, 0.f, 0.f ) },
        { L"Models/Library.pmd", L"", XMFLOAT3( 0.f, 1.f, 0.f ) },
        { L"Models/m_GUMI.zip", motionPath, XMFLOAT3( 10.f, 0.f, 0.f ) },
        { L"Models/Lat式ミクVer2.31_White.pmd", motionPath, XMFLOAT3( -11.f, 10.f, -19.f ) },
        { L"Models/Lat式ミクVer2.31_Normal.pmd", motionPath, XMFLOAT3( 0.f, 0.f, 10.f ) },
#endif
    };

#ifdef _DEBUG
    m_Models.emplace_back( std::make_shared<Graphics::GroundPlane>() );
#endif

	const std::wstring cameraPath = L"Motions/camera.vmd";
	m_Motion.LoadMotion( cameraPath );

	// Depth-only (2x rate)
    GraphicsPSO& DepthPSO = m_DepthPSO[kModelPMX];
	DepthPSO.SetRasterizerState( RasterizerDefault );
	DepthPSO.SetBlendState( BlendNoColorWrite );
	DepthPSO.SetInputLayout( static_cast<UINT>(Pmx::InputDescriptor.size()), Pmx::InputDescriptor.data() );
	DepthPSO.SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    DepthPSO.SetDepthStencilState( DepthStateReadWrite );
    DepthPSO.SetVertexShader( MY_SHADER_ARGS( g_pPmxDepthViewerVS ) );
    DepthPSO.Finalize();

    m_DepthPSO[kModelPMD] = DepthPSO;
	m_DepthPSO[kModelPMD].SetInputLayout( static_cast<UINT>(Pmd::InputDescriptor.size()), Pmd::InputDescriptor.data() );
    m_DepthPSO[kModelPMD].SetVertexShader( MY_SHADER_ARGS( g_pPmxDepthViewerVS ) );
    m_DepthPSO[kModelPMD].Finalize();

    // Depth-only but with a depth bias and/or render only backfaces
    for (int i = kModelPMD; i <= kModelPMX; i++)
    {
        m_ShadowPSO[i] = m_DepthPSO[i];
        m_ShadowPSO[i].SetRasterizerState( RasterizerShadow );
        m_ShadowPSO[i].Finalize();
    }

    m_DepthPSO[kModelGRD] = DepthPSO;
	m_DepthPSO[kModelGRD].SetInputLayout( static_cast<UINT>(Graphics::GroundPlanInputDesc.size()), Graphics::GroundPlanInputDesc.data() );

    for (int i = kModelPMD; i <= kModelGRD; i++)
    {
        m_OpaquePSO[i] = m_DepthPSO[i];
        m_OpaquePSO[i].SetBlendState( BlendDisable );
    }
    m_OpaquePSO[kModelPMD].SetVertexShader( MY_SHADER_ARGS( g_pPmdOpaqueVS ) );
    m_OpaquePSO[kModelPMD].SetPixelShader( MY_SHADER_ARGS( g_pPmdOpaquePS ) );;
    m_OpaquePSO[kModelPMX].SetVertexShader( MY_SHADER_ARGS( g_pPmxOpaqueVS ) );
    m_OpaquePSO[kModelPMX].SetPixelShader( MY_SHADER_ARGS( g_pPmxOpaquePS ) );;
    m_OpaquePSO[kModelGRD].SetVertexShader( MY_SHADER_ARGS( g_pGroundOpaqueVS ) );
    m_OpaquePSO[kModelGRD].SetPixelShader( MY_SHADER_ARGS( g_pGroundOpaquePS ) );;
    for (int i = kModelPMD; i <= kModelGRD; i++)
        m_OpaquePSO[i].Finalize();

    for (int i = kModelPMD; i <= kModelGRD; i++)
    {
        m_BlendPSO[i] = m_OpaquePSO[i];
        m_BlendPSO[i].SetRasterizerState( RasterizerDefault );
        m_BlendPSO[i].SetBlendState( BlendTraditional );
        m_BlendPSO[i].Finalize();
    }

	m_pCameraController = new MikuCameraController(m_Camera, Vector3(kYUnitVector));
	m_pSecondCameraController = new CameraController(m_SecondCamera, Vector3(kYUnitVector));

    m_ShadowViewProj.resize( kShadowSplit );
    m_ViewFrustum.resize( kShadowSplit );
    m_SplitFrustum.resize( kShadowSplit );

    MotionBlur::Enable = true;
    TemporalEffects::EnableTAA = false;
    FXAA::Enable = true;
    // PostEffects::EnableHDR = true;
    // PostEffects::EnableAdaptation = true;
    // SSAO::Enable = true;
}

void MikuViewer::Cleanup( void )
{
    m_Models.clear();
    ModelBase::Shutdown();
    Lighting::Shutdown();

	m_DepthPSO[kModelPMD].Destroy();
	m_DepthPSO[kModelPMX].Destroy();
	m_ShadowPSO[kModelPMD].Destroy();
	m_ShadowPSO[kModelPMX].Destroy();
    for (int i = 0; i < kModelMAX; i++)
    {
        m_OpaquePSO[i].Destroy();
        m_BlendPSO[i].Destroy();
    }

    Utility::DeleteObject( m_pCameraController );
    Utility::DeleteObject( m_pSecondCameraController  );
}

namespace Graphics
{
	extern EnumVar DebugZoom;
}

namespace GameCore
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	extern HWND g_hWnd;
#else
	extern Platform::Agile<Windows::UI::Core::CoreWindow> g_window;
#endif
}

MikuCamera* MikuViewer::SelectedCamera()
{
    if (m_CameraType == kCameraVirtual)
        return &m_SecondCamera;
    else
        return &m_Camera;
}

void MikuViewer::Update( float deltaT )
{
    using namespace Lighting;

    ScopedTimer _prof( L"Update" );

	if (GameInput::IsFirstPressed(GameInput::kLShoulder))
		DebugZoom.Decrement();
	else if (GameInput::IsFirstPressed(GameInput::kRShoulder))
		DebugZoom.Increment();

	// We use viewport offsets to jitter our color samples from frame to frame (with TAA.)
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

    for (auto& model : m_Models)
        model->Update( m_Frame );
	m_Motion.Update( m_Frame );

    m_Motion.Animate( m_Camera );
    if (m_CameraType == kCameraMain)
        m_pCameraController->Update( deltaT );
    else
        m_pSecondCameraController->Update( deltaT );

	m_ViewMatrix = SelectedCamera()->GetViewMatrix();
	m_ProjMatrix = SelectedCamera()->GetProjMatrix();

    m_SunDirection = Vector3( m_SunDirX, m_SunDirY, m_SunDirZ );
    m_SunColor = Vector3( m_SunColorR, m_SunColorG, m_SunColorB );
}

void MikuViewer::RenderObjects( GraphicsContext& gfxContext, const Matrix4& ViewMat, const Matrix4& ProjMat, eObjectFilter Filter )
{
    const int MaxSplit = 4;
    struct VSConstants
    {
        Matrix4 view;
        Matrix4 projection;
        Matrix4 shadow[MaxSplit];
    } vsConstants;
    vsConstants.view = ViewMat;
    vsConstants.projection = ProjMat;

    const Matrix4 T = AffineTransform( Matrix3::MakeScale( 0.5f, -0.5f, 1.0f ), Vector3( 0.5f, 0.5f, 0.0f ) );
    for (int i = 0; i < kShadowSplit; i++)
        vsConstants.shadow[i] = T * m_ShadowViewProj[i];
	gfxContext.SetDynamicConstantBufferView( 0, sizeof(vsConstants), &vsConstants, { kBindVertex } );
    for (auto& model : m_Models)
        model->Draw( gfxContext, Filter );
}

BoundingBox MikuViewer::GetBoundingBox()
{
    Vector3 minVec( FLT_MAX ), maxVec( FLT_MIN );
    for (auto& model : m_Models)
    {
        auto bound = model->GetBoundingBox();
        minVec = Min(bound.GetMin(), minVec);
        maxVec = Max(bound.GetMax(), maxVec);
    }
    return BoundingBox( minVec, maxVec );
}

void MikuViewer::RenderObjects( GraphicsContext& gfxContext, const Matrix4& ViewProjMat, eObjectFilter Filter )
{
    RenderObjects( gfxContext, ViewProjMat, Matrix4(kIdentity), Filter );
}

void MikuViewer::RenderLightShadows( GraphicsContext& gfxContext )
{
    using namespace Lighting;

    ScopedTimer _prof(L"RenderLightShadows", gfxContext);

    static uint32_t LightIndex = 0;
    if (LightIndex >= kMaxLights)
        return;

    m_LightShadowTempBuffer.BeginRendering(gfxContext);
    {
        gfxContext.SetPipelineState(m_ShadowPSO[kModelPMX]);
    }
    m_LightShadowTempBuffer.EndRendering(gfxContext);

    gfxContext.CopySubresource(m_LightShadowArray, LightIndex, m_LightShadowTempBuffer, 0);

    ++LightIndex;
}

// Makes the "global" shadow matrix used as the reference point for the cascades
Matrix4 MakeGlobalShadowMatrix(const BaseCamera& camera, Vector3 LightDirection)
{
    // Get the 8 points of the view frustum in world space
    Vector3 frustumCorners[8] =
    {
        Vector3(-1.0f, -1.0f, 1.0f),
        Vector3(-1.0f,  1.0f, 1.0f),
        Vector3( 1.0f, -1.0f, 1.0f),
        Vector3( 1.0f,  1.0f, 1.0f),
        Vector3(-1.0f, -1.0f, 0.0f),
        Vector3(-1.0f,  1.0f, 0.0f),
        Vector3( 1.0f, -1.0f, 0.0f),
        Vector3( 1.0f,  1.0f, 0.0f),
    };

    const Matrix4& invViewProj = camera.GetClipToWorld();
    Vector3 frustumCenter = 0.0f;
    for(uint32_t i = 0; i < 8; ++i)
    {
        frustumCorners[i] = invViewProj.Transform( frustumCorners[i] );
        frustumCenter += frustumCorners[i];
    }

    frustumCenter /= Scalar(8.0f);

    // Pick the up vector to use for the light camera
    Vector3 upDir = camera.GetRightVec();

    // This needs to be constant for it to be stable
    if (m_bStabilizeCascades)
        upDir = Vector3(0.0f, 1.0f, 0.0f);

    // Get position of the shadow camera
    Vector3 shadowCameraPos = frustumCenter + LightDirection * 0.5f;

    // Come up with a new orthographic camera for the shadow caster
    OrthographicCamera lightCamera;
    lightCamera.SetOrthographic( -0.5f, 0.5f, -0.5f, 0.5f, 0.0f, 1.0f );
    lightCamera.SetEyeAtUp( shadowCameraPos, frustumCenter, upDir );
    lightCamera.Update();

    Matrix4 Tex = AffineTransform( Matrix3::MakeScale( 0.5f, -0.5f, 1.0f ), Vector3( 0.5f, 0.5f, 0.0f ) );
    return Tex * lightCamera.GetViewProjMatrix();
}

void MikuViewer::RenderShadowMap( GraphicsContext& gfxContext )
{
    ScopedTimer _prof( L"Render Shadow Map", gfxContext );

    m_SunShadow.UpdateMatrix( m_SunDirection, Vector3(0.f), Vector3(m_ShadowDimX, m_ShadowDimY, m_ShadowDimZ),
        (uint32_t)g_ShadowBuffer.GetWidth(), (uint32_t)g_ShadowBuffer.GetHeight(), 16 );

    Matrix4 lightView = m_SunShadow.GetViewMatrix();
    BoundingBox bound = GetBoundingBox();
    FrustumCorner boundFrustum = bound.GetCorners();
    for ( auto i = 0; i < boundFrustum.size(); i++ )
        boundFrustum[i] = Vector3(lightView * boundFrustum[i]);

    const float sMapSize = static_cast<float>(g_CascadeShadowBuffer.GetWidth());

    // MinCascadeDistance.Initialize(tweakBar, "MinCascadeDistance", "CascadeControls", "Min Cascade Distance", "The closest depth that is covered by the shadow cascades", 0.0000f, 0.0000f, 0.1000f, 0.0010f);
    // MaxCascadeDistance.Initialize(tweakBar, "MaxCascadeDistance", "CascadeControls", "Max Cascade Distance", "The furthest depth that is covered by the shadow cascades", 1.0000f, 0.0000f, 1.0000f, 0.0100f);
    const float MinDistance = 0.0f;
    const float MaxDistance = 1.f;

    // Compute the split distances based on the partitioning mode
    float CascadeSplits[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    float m_SplitDistance0 = 0.002f, m_SplitDistance1 = 0.006f, m_SplitDistance2 = 0.01f, m_SplitDistance3 = 0.1f;
    if (true)
    {
        CascadeSplits[0] = MinDistance + m_SplitDistance0 * MaxDistance;
        CascadeSplits[1] = MinDistance + m_SplitDistance1 * MaxDistance;
        CascadeSplits[2] = MinDistance + m_SplitDistance2 * MaxDistance;
        CascadeSplits[3] = MinDistance + m_SplitDistance3 * MaxDistance;
    }

    // Render the meshes to each cascade
    uint32_t NumCascades = kShadowSplit;
    for (uint32_t cascadeIdx = 0; cascadeIdx < NumCascades; ++cascadeIdx)
    {
        float prevSplitDist = cascadeIdx == 0 ? MinDistance : CascadeSplits[cascadeIdx - 1];
        float splitDist = CascadeSplits[cascadeIdx];

        // Get the 8 points of the view frustum in world space
        Vector3 frustumCornersWS[8] =
        {
            Vector3(-1.0f, -1.0f, 1.0f),
            Vector3(-1.0f,  1.0f, 1.0f),
            Vector3( 1.0f, -1.0f, 1.0f),
            Vector3( 1.0f,  1.0f, 1.0f),

            Vector3(-1.0f, -1.0f, 0.0f),
            Vector3(-1.0f,  1.0f, 0.0f),
            Vector3( 1.0f, -1.0f, 0.0f),
            Vector3( 1.0f,  1.0f, 0.0f),
        };

        Matrix4 invViewProj = m_Camera.GetClipToWorld();
        for(uint32_t i = 0; i < 8; ++i)
            frustumCornersWS[i] = invViewProj.Transform(frustumCornersWS[i]);

        // Get the corners of the current cascade slice of the view frustum
        for (uint32_t i = 0; i < 4; ++i)
        {
            Vector3 cornerRay = frustumCornersWS[i + 4] - frustumCornersWS[i];
            Vector3 nearCornerRay = cornerRay * prevSplitDist;
            Vector3 farCornerRay = cornerRay * splitDist;
            frustumCornersWS[i] = frustumCornersWS[i + 4] - farCornerRay;
            frustumCornersWS[i + 4] = frustumCornersWS[i + 4] - nearCornerRay;
        }

        std::copy( frustumCornersWS, frustumCornersWS + 8, m_ViewFrustum[cascadeIdx].begin() );

        // Calculate the centroid of the view frustum slice
        Vector3 frustumCenter = 0.0f;
        for(uint32_t i = 0; i < 8; ++i)
            frustumCenter = frustumCenter + frustumCornersWS[i];
        frustumCenter *= Scalar( 1.0f / 8.0f );

        Vector3 minExtents;
        Vector3 maxExtents;
        if (m_bStabilizeCascades)
        {
            // Calculate the radius of a bounding sphere surrounding the frustum corners
            float sphereRadius = 0.0f;
            for (uint32_t i = 0; i < 8; ++i)
            {
                float dist = Length( frustumCornersWS[i] - frustumCenter );
                sphereRadius = std::max( sphereRadius, dist );
            }

            sphereRadius = std::ceil( sphereRadius * 16.0f ) / 16.0f;

            maxExtents = Vector3( sphereRadius, sphereRadius, sphereRadius );
            minExtents = -maxExtents;
        }
        else
        {
            Vector3 mins( Scalar( FLT_MAX ) );
            Vector3 maxes( Scalar( -FLT_MAX ) );
            for (uint32_t i = 0; i < 8; ++i)
            {
                Vector3 corner = lightView.Transform( frustumCornersWS[i] );
                mins = Min( mins, corner );
                maxes = Max( maxes, corner );
            }
            minExtents = mins;
            maxExtents = maxes;
        }

        Vector3 cascadeExtents = maxExtents - minExtents;

        // Come up with a new orthographic camera for the shadow caster
        OrthographicCamera shadowCamera;
        shadowCamera.SetOrthographic( minExtents.GetX(), maxExtents.GetX(), minExtents.GetY(), maxExtents.GetY(), minExtents.GetZ(), maxExtents.GetZ() );
        shadowCamera.SetEyeAtUp( Vector3(0.f), m_SunDirection, Vector3( kYUnitVector ) );
        shadowCamera.Update();

        m_SplitFrustum[cascadeIdx] = shadowCamera.GetWorldSpaceFrustum().GetFrustumCorners();
        m_ShadowViewProj[cascadeIdx] = shadowCamera.GetViewProjMatrix();

        // Draw the mesh with depth only, using the new shadow camera
        g_CascadeShadowBuffer.BeginRendering( gfxContext, cascadeIdx );
        gfxContext.SetPipelineState( m_ShadowPSO[kModelPMX] );
        RenderObjects( gfxContext, shadowCamera.GetViewProjMatrix(), kOpaque );
        RenderObjects( gfxContext, shadowCamera.GetViewProjMatrix(), kTransparent );
        g_CascadeShadowBuffer.EndRendering( gfxContext );
    }
}

void MikuViewer::RenderScene( void )
{
    using namespace Lighting;

	GraphicsContext& gfxContext = GraphicsContext::Begin( L"Scene Render" );

    uint32_t FrameIndex = TemporalEffects::GetFrameIndexMod2();
    (FrameIndex);

    __declspec(align(16)) struct
    {
        Vector3 LightDirection;
        Vector3 LightColor;
        float ShadowTexelSize[4];
    } psConstants;

    psConstants.LightDirection = m_Camera.GetViewMatrix().Get3x3() * m_SunDirection;
    psConstants.LightColor = m_SunColor / Vector3( 255.f, 255.f, 255.f );
    psConstants.ShadowTexelSize[0] = 1.0f / g_ShadowBuffer.GetWidth();
    psConstants.ShadowTexelSize[1] = 1.0f / g_ShadowBuffer.GetHeight();
	gfxContext.SetDynamicConstantBufferView( 1, sizeof(psConstants), &psConstants, { kBindPixel } );

    D3D11_SAMPLER_HANDLE Sampler[] = { SamplerLinearWrap, SamplerLinearClamp, SamplerShadow };
    gfxContext.SetDynamicSamplers( 0, _countof(Sampler), Sampler, { kBindPixel } );

    RenderLightShadows(gfxContext);
    RenderShadowMap(gfxContext);

	gfxContext.SetDynamicConstantBufferView( 1, sizeof(psConstants), &psConstants, { kBindPixel } );

    gfxContext.ClearColor( g_SceneColorBuffer );
    gfxContext.ClearDepth( g_SceneDepthBuffer );
    {
        ScopedTimer _prof( L"Render Color", gfxContext );

        gfxContext.SetDynamicDescriptor( 4, g_CascadeShadowBuffer.GetSRV(), { kBindPixel } );
        gfxContext.SetViewportAndScissor( m_MainViewport, m_MainScissor );
        gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
        gfxContext.SetPipelineState( m_OpaquePSO[kModelPMX] );
        RenderObjects( gfxContext, m_ViewMatrix, m_ProjMatrix, kOpaque );
        RenderObjects( gfxContext, m_ViewMatrix, m_ProjMatrix, kOverlay );
        ModelBase::Flush( gfxContext );
        gfxContext.SetPipelineState( m_BlendPSO[kModelPMX] );
        RenderObjects( gfxContext, m_ViewMatrix, m_ProjMatrix, kTransparent );
    }
    {
        ScopedTimer _prof( L"Render Frustum", gfxContext );

        const std::vector<Color> SplitColor =
        {
            Color(1.f, 0.f, 0.f, 0.36f),
            Color(0.f, 1.f, 0.f, 0.36f),
            Color(0.f, 0.f, 1.f, 0.36f),
            Color(1.f, 1.f, 0.f, 0.36f)
        };

        auto WorldToClip = m_ProjMatrix * m_ViewMatrix;
        if (m_bShadowSplit)
        {
            for (auto i = 0; i < m_ShadowViewProj.size(); i++)
                Utility::DebugCube( gfxContext, WorldToClip, m_SplitFrustum[i].data(), SplitColor[i] );
        }
        if (m_bViewSplit)
        {
            for (auto i = 0; i < m_ViewFrustum.size(); i++)
                Utility::DebugCube( gfxContext, WorldToClip, m_ViewFrustum[i].data(), SplitColor[i] );
        }

        if (m_bLightFrustum)
        {
            Matrix4 FixDepth( kIdentity );
            FixDepth.SetZ( Vector4(kZero) );
            FixDepth.SetW( Vector4(0.f, 0.f, 0.0f, 1.f) );
            auto Corners  = m_SunShadow.GetWorldSpaceFrustum().GetFrustumCorners();
            Utility::DebugCube( gfxContext, FixDepth * WorldToClip, Corners.data(), Color( 0.f, 1.f, 1.f, 0.1f ) );
        }
    }
    if (m_bDebugTexture)
    {
        ScopedTimer _prof( L"DebugTexture", gfxContext );
        std::pair<int, int> pos[] = { {0, 0}, {500, 0}, {0, 500}, {500, 500} };
        for (uint32_t i = 0; i < _countof(pos); i++)
            Utility::DebugTexture( gfxContext, g_CascadeShadowBuffer.GetSRV( i ), pos[i].first, pos[i].second );
    }
    gfxContext.SetRenderTarget( nullptr );
    TemporalEffects::ResolveImage(gfxContext);

	gfxContext.Finish();
}

void MikuViewer::RenderUI( GraphicsContext& Context )
{
	auto Pos = SelectedCamera()->GetPosition();
	auto x = (float)g_OverlayBuffer.GetWidth() - 400.f;
    float px = Pos.GetX(), py = Pos.GetY(), pz = Pos.GetZ();

	TextContext Text(Context);
	Text.Begin();
	Text.ResetCursor( x, 10.f );
    Text.DrawFormattedString( "Position: (%.1f, %.1f, %.1f)\n", px, py, pz );
	Text.End();
}