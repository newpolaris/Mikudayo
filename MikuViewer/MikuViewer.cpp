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
#include "MikuModel.h"
#include "GroundPlane.h"
#include "Shadow.h"

#include "CompiledShaders/MikuModelVS.h"
#include "CompiledShaders/MikuModelPS.h"
#include "CompiledShaders/DepthViewerVS.h"
#include "CompiledShaders/DepthViewerPS.h"
#include "CompiledShaders/GroundPlaneVS.h"
#include "CompiledShaders/GroundPlanePS.h"

using namespace DirectX;
using namespace GameCore;
using namespace Graphics;
using namespace Math;

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

using FrustumCorner = std::array<Vector3, 8>;

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

    void RenderObjects( GraphicsContext& gfxContext, const Matrix4& ViewProjMat, eObjectFilter Filter );
    void RenderObjects( GraphicsContext& gfxContext, const Matrix4 & ViewMat, const Matrix4 & ProjMat, eObjectFilter Filter );
    void RenderLightShadows(GraphicsContext& gfxContext);
    void RenderShadowMap(GraphicsContext& gfxContext);
    MikuCamera* SelectedCamera();

	MikuCamera m_Camera;
    MikuCamera m_SecondCamera;
	MikuCameraController* m_pCameraController;

    Matrix4 m_ViewMatrix;
    Matrix4 m_ProjMatrix;

	D3D11_VIEWPORT m_MainViewport;
	float m_JitterDelta[2];
	D3D11_RECT m_MainScissor;

    Vector3 m_SunDirection;
    Vector3 m_SunColor;
    ShadowCamera m_SunShadow;
    D3D11_SAMPLER_HANDLE m_SamplerShadow;

    std::vector<std::shared_ptr<Graphics::IRenderObject>> m_Models;
	Graphics::Motion m_Motion;

	GraphicsPSO m_DepthPSO; 
    GraphicsPSO m_CutoutDepthPSO;
    GraphicsPSO m_ShadowPSO;
    GraphicsPSO m_CutoutShadowPSO;
	GraphicsPSO m_OpaquePSO;
	GraphicsPSO m_BlendPSO;
	GraphicsPSO m_GroundPlanePSO;

    std::vector<Matrix4> m_SplitViewProjs;
    std::vector<FrustumCorner> m_SplitViewFrustum;
};

CREATE_APPLICATION( MikuViewer )

NumVar m_Frame( "Application/Animation/Frame", 0, 0, 1e5, 1 );

enum { kCameraMain, kCameraVirtual };
const char* CameraNames[] = { "CameraMain", "CameraVirtual" };
EnumVar m_CameraType("Application/Camera/Camera Type", kCameraMain, kCameraVirtual+1, CameraNames );
BoolVar m_bDebugTexture("Application/Camera/Debug Texture", true);
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

MikuViewer::MikuViewer() : m_pCameraController( nullptr )
{
    // g_CascadeShadowBuffer.SetDepthFormat( DXGI_FORMAT_D32_FLOAT );
    g_CascadeShadowBuffer.SetClearDepth( BaseCamera::GetClearDepth() );
	g_SceneDepthBuffer.SetClearDepth( BaseCamera::GetClearDepth() );
	g_ShadowBuffer.SetClearDepth( BaseCamera::GetClearDepth() );
}

void MikuViewer::Startup( void )
{
	TextureManager::Initialize( L"Textures" );
    Lighting::Initialize();

    struct ModelInit
    {
        std::wstring Model;
        std::wstring Motion;
        XMFLOAT3 Position;
    };

    auto motionPath = L"Models/nekomimi_lat.vmd";
    std::vector<ModelInit> list = {
        { L"Models/Lat0.pmd", motionPath, XMFLOAT3( -10.f, 0.f, 0.f ) },
#ifndef _DEBUG
        { L"Models/Library.pmd", L"", XMFLOAT3( 0.f, 1.f, 0.f ) },
        { L"Models/m_GUMI.zip", motionPath, XMFLOAT3( 10.f, 0.f, 0.f ) },
        { L"Models/Lat式ミクVer2.31_White.pmd", motionPath, XMFLOAT3( -11.f, 10.f, -19.f ) },
        { L"Models/Lat式ミクVer2.31_Normal.pmd", motionPath, XMFLOAT3( 0.f, 0.f, 10.f ) },
#endif
    };

    for (auto l : list)
    {
        auto model = std::make_shared<Graphics::MikuModel>();
        model->SetModel( l.Model );
        model->SetMotion( l.Motion );
        model->SetPosition( l.Position );
        model->Load();
        m_Models.push_back( model );
    }
#ifdef _DEBUG
    m_Models.emplace_back( std::make_shared<Graphics::GroundPlane>() );
#endif

	const std::wstring cameraPath = L"Models/camera.vmd";
	m_Motion.LoadMotion( cameraPath );

    D3D11_DEPTH_STENCIL_DESC& DepthReadWrite = Camera::GetReverseZ() ? DepthStateReadWrite : DepthStateReadWriteLE;
    float Sign = Camera::GetReverseZ() ? -1.f : 1.f;
    for (auto Desc : {&RasterizerShadow, &RasterizerShadowCW, &RasterizerShadowTwoSided})
    {
        Desc->SlopeScaledDepthBias = Sign * 2.0f;
        Desc->DepthBias = static_cast<INT>(Sign * 0);
    }

	// Depth-only (2x rate)
	m_DepthPSO.SetRasterizerState( RasterizerDefault );
	m_DepthPSO.SetBlendState( BlendNoColorWrite );
	m_DepthPSO.SetInputLayout( static_cast<UINT>(Pmd::InputDescriptor.size()), Pmd::InputDescriptor.data() );
	m_DepthPSO.SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    m_DepthPSO.SetDepthStencilState( DepthReadWrite );
    m_DepthPSO.SetVertexShader( MY_SHADER_ARGS( g_pDepthViewerVS ) );
    m_DepthPSO.Finalize();

    // Depth-only shading but with alpha testing
    m_CutoutDepthPSO = m_DepthPSO;
    m_CutoutDepthPSO.SetPixelShader( MY_SHADER_ARGS( g_pDepthViewerPS ) );
    m_CutoutDepthPSO.SetRasterizerState(RasterizerTwoSided);
    m_CutoutDepthPSO.Finalize();

    // Depth-only but with a depth bias and/or render only backfaces
    m_ShadowPSO = m_DepthPSO;
    m_ShadowPSO.SetRasterizerState( RasterizerShadow );
    m_ShadowPSO.Finalize();

    // Shadows with alpha testing
    m_CutoutShadowPSO = m_ShadowPSO;
    m_CutoutShadowPSO.SetPixelShader( MY_SHADER_ARGS( g_pDepthViewerPS ) );
    m_CutoutShadowPSO.SetRasterizerState(RasterizerShadowTwoSided);
    m_CutoutShadowPSO.Finalize();

	m_GroundPlanePSO = m_DepthPSO;
	m_GroundPlanePSO.SetInputLayout( static_cast<UINT>(Graphics::GroundPlanInputDesc.size()), Graphics::GroundPlanInputDesc.data() );
	m_GroundPlanePSO.SetBlendState( BlendDisable );
	m_GroundPlanePSO.SetVertexShader( MY_SHADER_ARGS( g_pGroundPlaneVS) );
	m_GroundPlanePSO.SetPixelShader( MY_SHADER_ARGS( g_pGroundPlanePS ) );
	m_GroundPlanePSO.Finalize();

	m_OpaquePSO = m_DepthPSO;
	m_OpaquePSO.SetBlendState( BlendDisable );
	m_OpaquePSO.SetVertexShader( MY_SHADER_ARGS( g_pMikuModelVS ) );
	m_OpaquePSO.SetPixelShader( MY_SHADER_ARGS( g_pMikuModelPS ) );
	m_OpaquePSO.Finalize();

	m_BlendPSO = m_OpaquePSO;
	m_BlendPSO.SetRasterizerState( RasterizerDefault );
	m_BlendPSO.SetBlendState( BlendTraditional );
	m_BlendPSO.Finalize();

	m_pCameraController = new MikuCameraController(m_Camera, Vector3(kYUnitVector));
	m_pCameraController->SetMotion( &m_Motion );

    m_SamplerShadow = BaseCamera::GetReverseZ() ? SamplerShadowGE : SamplerShadowLE;

    using namespace Lighting;
    m_SplitViewProjs.resize( 4 );
    m_SplitViewFrustum.resize( 4 );

    MotionBlur::Enable = true;
    TemporalEffects::EnableTAA = false;
    FXAA::Enable = true;
    // PostEffects::EnableHDR = true;
    // PostEffects::EnableAdaptation = true;
    // SSAO::Enable = true;
}

void MikuViewer::Cleanup( void )
{
    Lighting::Shutdown();

	m_DepthPSO.Destroy(); 
    m_CutoutDepthPSO.Destroy();
    m_ShadowPSO.Destroy();
    m_CutoutShadowPSO.Destroy();
	m_OpaquePSO.Destroy();
	m_BlendPSO.Destroy();
    m_GroundPlanePSO.Destroy();
    m_Models.clear();

	delete m_pCameraController;
	m_pCameraController = nullptr;
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

    m_pCameraController->HandOverControl( SelectedCamera() );
	m_pCameraController->Update( deltaT );

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

    auto T = Matrix4( AffineTransform( Matrix3::MakeScale( 0.5f, -0.5f, 1.0f ), Vector3( 0.5f, 0.5f, 0.0f ) ) );
    for (int i = 0; i < 4; i++)
        vsConstants.shadow[i] = T * m_SplitViewProjs[i] * m_SunShadow.GetViewMatrix();

	gfxContext.SetDynamicConstantBufferView( 0, sizeof(vsConstants), &vsConstants, { kBindVertex } );

    for (auto& model : m_Models)
        model->Draw( gfxContext, Filter );
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
        gfxContext.SetPipelineState(m_ShadowPSO);
        gfxContext.SetPipelineState(m_CutoutShadowPSO);
    }
    m_LightShadowTempBuffer.EndRendering(gfxContext);

    gfxContext.CopySubresource(m_LightShadowArray, LightIndex, m_LightShadowTempBuffer, 0);

    ++LightIndex;
}

//
// Returns 8 clipspace frustum corners
//
std::vector<Vector3> GetClipCorners( float Near, float Far )
{
    float Left = -1.f, Right = 1.f, Bottom = -1.f, Top = 1.f;
    return {
        Vector3( Left, Bottom, Near ),	// Near lower left
        Vector3( Left, Top, Near ),	    // Near upper left
        Vector3( Right, Bottom, Near ),	// Near lower right
        Vector3( Right, Top, Near ),	// Near upper right
        Vector3( Left, Bottom, Far ),	// Far lower left
        Vector3( Left, Top, Far ),	    // Far upper left
        Vector3( Right, Bottom, Far ),	// Far lower right
        Vector3( Right, Top, Far ),	    // Far upper right
    };
}

class BoundingBox
{
public:
    BoundingBox(Vector3 MinVec, Vector3 MaxVec) 
    {
        m_Center = 0.5f * (MaxVec + MinVec);
        m_Extent = 0.5f * (MaxVec - MinVec);
    }

    std::array<Vector3, 8> GetCorners( void ) const;

    Vector3 m_Center;
    Vector3 m_Extent;
};

std::array<Vector3, 8> BoundingBox::GetCorners( void ) const
{
    const auto x = m_Extent.GetX(), y = m_Extent.GetY(), z = m_Extent.GetZ();
    const auto cx = m_Center.GetX(), cy = m_Center.GetY(), cz = m_Center.GetZ();

    // kNearLowerLeft, kNearUpperLeft, kNearLowerRight, kNearUpperRight,
    // kFarLowerLeft, kFarUpperLeft, kFarLowerRight, kFarUpperRight
    return {
        Vector3( cx - x, cy - y, cz + z ),
        Vector3( cx - x, cy + y, cz + z ),
        Vector3( cx + x, cy - y, cz + z ),
        Vector3( cx + x, cy + y, cz + z ),

        Vector3( cx - x, cy - y, cz - z ),
        Vector3( cx - x, cy + y, cz - z ),
        Vector3( cx + x, cy - y, cz - z ),
        Vector3( cx + x, cy + y, cz - z )
    };
}

void MikuViewer::RenderShadowMap( GraphicsContext& gfxContext )
{
    ScopedTimer _prof( L"Render Shadow Map", gfxContext );

    // m_SunShadow.UpdateMatrix( m_Camera.GetForwardVec(), m_Camera.GetPosition(), Vector3( m_ShadowDimX, m_ShadowDimY, m_ShadowDimZ ),
    // m_SunShadow.UpdateMatrix( m_SunDirection, -m_SunDirection * 200, Vector3( m_ShadowDimX, m_ShadowDimY, m_ShadowDimZ ),
    m_SunShadow.UpdateMatrix( m_SunDirection, Vector3( 0, 0, 0 ), Vector3( m_ShadowDimX, m_ShadowDimY, m_ShadowDimZ ),
        (uint32_t)g_ShadowBuffer.GetWidth(), (uint32_t)g_ShadowBuffer.GetHeight(), 16 );

    const Matrix4& Proj = m_Camera.GetProjMatrix();

    std::vector<float> ViewSpaceDepth = { -1.0f, -30.f, -80.f, -150.f, -300.f };
    std::vector<float> ClipSpaceDepth( ViewSpaceDepth.size() );
    std::transform(ViewSpaceDepth.begin(), ViewSpaceDepth.end(), ClipSpaceDepth.begin(), 
        [&Proj]( float d ) {
        Vector4 c = Proj * Vector4( 0.f, 0.f, d, 1.f );
        if (c.GetW() < 0.0001f)
            return Scalar(0.f);
        return c.GetZ() / c.GetW();
    });

    const Matrix4& ClipToWorld = m_Camera.GetClipToWorld();
    for (auto i = 0; i + 1 < ClipSpaceDepth.size(); i++)
    {
        // Get clip-space frustum corners for near and far, transform into shadow view space
        auto ClipCorners = GetClipCorners( ClipSpaceDepth[i], ClipSpaceDepth[i + 1] );
        std::transform( ClipCorners.begin(), ClipCorners.end(), m_SplitViewFrustum[i].begin(),
            [&ClipToWorld]( Vector3 v ) {
            Vector4 vt = ClipToWorld * Vector4(v, 1.f);
            return Vector3(vt);
        } );
    }

    auto& ShadowView = m_SunShadow.GetViewMatrix();
    for (auto cascade = 0; cascade < m_SplitViewFrustum.size(); cascade++)
    {
        auto& frustumCornersWS = m_SplitViewFrustum[cascade];

        // Calculate the centroid of the view frustum slice
        Vector3 frustumCenter = 0.0f;
        for (uint32_t i = 0; i < 8; ++i)
            frustumCenter = frustumCenter + frustumCornersWS[i];
        frustumCenter *= Scalar(1.0f / 8.0f);

        // Pick the up vector to use for the light camera
        Vector3 upDir = m_Camera.GetRightVec(); // camera.Right();

        Vector3 minExtents;
        Vector3 maxExtents;
        if (m_bStabilizeCascades)
        {
            // This needs to be constant for it to be stable
            upDir = Vector3( kYUnitVector );

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
                Vector3 corner = ShadowView.Transform( frustumCornersWS[i] );
                mins = Min( mins, corner );
                maxes = Max( maxes, corner );
            }
            minExtents = mins;
            maxExtents = maxes;
        }

        Vector3 cascadeExtents = maxExtents - minExtents;

        // Get position of the shadow camera
        Vector3 shadowCameraPos = frustumCenter + m_SunDirection * -minExtents.GetZ();

        float minZ = minExtents.GetZ(), maxZ = maxExtents.GetZ();
        if (m_bFixDepth)
            minZ = 0.0f, maxZ = cascadeExtents.GetZ();

        // Come up with a new orthographic camera for the shadow caster
        Matrix4 SplitProj = Matrix4( XMMatrixOrthographicOffCenterRH(
            minExtents.GetX(), maxExtents.GetX(), 
            minExtents.GetY(), maxExtents.GetY(), 
            minZ, maxZ ) );

        Matrix4 View = Matrix4( XMMatrixLookAtRH( shadowCameraPos, frustumCenter, upDir ) );
        m_SplitViewProjs[cascade] = SplitProj;
    }

    for (auto i = 0; i < m_SplitViewProjs.size(); i++)
    {
        g_CascadeShadowBuffer.BeginRendering( gfxContext, i );
        gfxContext.SetPipelineState( m_ShadowPSO );
        RenderObjects( gfxContext, m_SplitViewProjs[i] * ShadowView, kOpaque );
        RenderObjects( gfxContext, m_SplitViewProjs[i] * ShadowView, kTransparent );
        g_CascadeShadowBuffer.EndRendering( gfxContext );
    }
}

void MikuViewer::RenderScene( void )
{
    using namespace Lighting;

	GraphicsContext& gfxContext = GraphicsContext::Begin( L"Scene Render" );

    uint32_t FrameIndex = TemporalEffects::GetFrameIndexMod2();
    (FrameIndex);

    struct 
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

    D3D11_SAMPLER_HANDLE Sampler[] = { SamplerLinearWrap, SamplerLinearClamp, m_SamplerShadow };
    gfxContext.SetDynamicSamplers( 0, _countof(Sampler), Sampler, { kBindPixel } );

    RenderLightShadows(gfxContext);
    RenderShadowMap(gfxContext);

    gfxContext.ClearColor( g_SceneColorBuffer );
    gfxContext.ClearDepth( g_SceneDepthBuffer );
    {
        ScopedTimer _prof( L"Render Color", gfxContext );

        gfxContext.SetDynamicDescriptor( 4, g_CascadeShadowBuffer.GetSRV(), { kBindPixel } );
        gfxContext.SetViewportAndScissor( m_MainViewport, m_MainScissor );
        gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
        gfxContext.SetPipelineState( m_OpaquePSO );
        RenderObjects( gfxContext, m_ViewMatrix, m_ProjMatrix, kOpaque );
        RenderObjects( gfxContext, m_ViewMatrix, m_ProjMatrix, kBone );
        gfxContext.SetPipelineState( m_BlendPSO );
        RenderObjects( gfxContext, m_ViewMatrix, m_ProjMatrix, kTransparent );
        gfxContext.SetPipelineState( m_GroundPlanePSO );
        RenderObjects( gfxContext, m_ViewMatrix, m_ProjMatrix, kGroundPlane );
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
            for (auto i = 0; i < m_SplitViewProjs.size(); i++)
            {
                Frustum FrustumVS( m_SplitViewProjs[i] );
                Frustum FrustumWS = m_SunShadow.GetCameraToWorld() * FrustumVS;
                Utility::DebugCube( gfxContext, WorldToClip, FrustumWS.GetFrustumCorners().data(), SplitColor[i] );
            }
        }
        if (m_bViewSplit)
        {
            for (auto i = 0; i < m_SplitViewFrustum.size(); i++)
                Utility::DebugCube( gfxContext, WorldToClip, m_SplitViewFrustum[i].data(), SplitColor[i] );
        }

        if (m_bLightFrustum)
        {
            Matrix4 FixDepth( kIdentity );
            FixDepth.SetZ( Vector4(kZero) );
            FixDepth.SetW( Vector4(0.f, 0.f, 0.0f, 1.f) );
            auto& WorldFrustum = m_SunShadow.GetWorldSpaceFrustum();
            auto Corners = WorldFrustum.GetFrustumCorners();
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
	auto pos = SelectedCamera()->GetPosition();
	auto x = (float)g_SceneColorBuffer.GetWidth() - 400.f;
    float px = pos.GetX(), py = pos.GetY(), pz = pos.GetZ();

	TextContext Text(Context);
	Text.Begin();
	Text.ResetCursor( x, 10.f );
    Text.DrawFormattedString( "Position: (%.1f, %.1f, %.1f)\n", px, py, pz );
	Text.End();
}