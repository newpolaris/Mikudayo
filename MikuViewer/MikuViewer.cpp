#include "GameCore.h"
#include "GraphicsCore.h"
#include "PipelineState.h"
#include "CommandContext.h"
#include "GpuBuffer.h"
#include "InputLayout.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "ConstantBuffer.h"
#include "TemporalEffects.h"
#include "TextureManager.h"
#include "BufferManager.h"
#include "MotionBlur.h"
#include "DepthOfField.h"
#include "MikuCamera.h"
#include "MikuCameraController.h"
#include "CameraController.h"
#include "GameInput.h"
#include "Motion.h"
#include "FXAA.h"

#include "CompiledShaders/ModelViewerVS.h"
#include "CompiledShaders/ModelViewerPS.h"
#include "CompiledShaders/MikuModel_VS.h"
#include "CompiledShaders/MikuModel_Skin_VS.h"

#include "DirectXColors.h"
#include <sstream>

#include "MikuModel.h"

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

// Constant buffer used to send MVP matrices to the vertex shader.
struct CameraConstants
{
	Matrix4 View;
	Matrix4 Projection;
};

struct DirectionalLight
{
	XMFLOAT3 Color;
	float pad;
	XMFLOAT3 Direction; // incident, I
};

__declspec(align(16)) struct LightsConstants
{
	enum { kMaxLight = 4 };
	DirectionalLight light[kMaxLight];
	uint32_t NumLight;
};

class MikuViewer : public GameCore::IGameApp
{
public:
	MikuViewer() : m_pCameraController( nullptr )
	{
	}

	virtual void Startup( void ) override;
	virtual void Cleanup( void ) override;

	virtual void Update( float deltaT ) override;
	virtual void RenderScene( void ) override;

    void RenderUI( GraphicsContext& Context ) override;

private:

	MikuCamera m_Camera;
	MikuCameraController* m_pCameraController;

	CameraConstants m_CameraData;
	ConstantBuffer<CameraConstants> m_Buffer;
	std::vector<DirectionalLight> m_Lights;
	LightsConstants m_LightConstants;

	D3D11_VIEWPORT m_MainViewport;
	float m_JitterDelta[2];
	D3D11_RECT m_MainScissor;

    std::vector<std::shared_ptr<Graphics::MikuModel>> m_Models;
	Graphics::Motion m_Motion;

	GraphicsPSO m_DepthPSO; 
	GraphicsPSO m_OpaquePSO;
	GraphicsPSO m_BlendPSO;
	GraphicsPSO m_OpaqueSkinPSO;
	GraphicsPSO m_BlendSkinPSO;
};

CREATE_APPLICATION( MikuViewer )

NumVar m_Frame( "Application/Animation/Frame", 0, 0, 1e5, 1 );

void MikuViewer::Startup( void )
{
	TextureManager::Initialize( L"Textures" );

    struct ModelInit
    {
        std::wstring Model;
        std::wstring Motion;
        XMFLOAT3 Position;
    };

    auto motionPath = L"Models/nekomimi_lat.vmd";
	const std::wstring cameraPath = L"Models/camera.vmd";

    std::vector<ModelInit> list = {
        { L"Models/gumi.pmd", motionPath, XMFLOAT3( 10.f, 0.f, 0.f ) },
        { L"Models/Lat0.pmd", motionPath, XMFLOAT3( -10.f, 0.f, 0.f ) },
        { L"Models/Lat式ミクVer2.31_White.pmd", motionPath, XMFLOAT3( -11.f, 10.f, -19.f ) },
        { L"Models/Lat式ミクVer2.31_Normal.pmd", motionPath, XMFLOAT3( 0.f, 0.f, 10.f ) },
        { L"Models/Library.pmd", L"", XMFLOAT3( 0.f, 1.f, 0.f ) }
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

	m_Motion.LoadMotion( cameraPath );

	// Depth-only (2x rate)
	m_DepthPSO.SetRasterizerState( RasterizerDefault );
	m_DepthPSO.SetBlendState( BlendNoColorWrite );
	m_DepthPSO.SetInputLayout( static_cast<UINT>(Pmd::InputDescriptor.size()), Pmd::InputDescriptor.data() );
	m_DepthPSO.SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_OpaquePSO = m_DepthPSO;
	m_OpaquePSO.SetBlendState( BlendDisable );
	
	if (m_Camera.GetReverseZ())
		m_OpaquePSO.SetDepthStencilState( DepthStateReadWrite );
	else
		m_OpaquePSO.SetDepthStencilState( DepthStateReadWriteLE );

	m_OpaquePSO.SetVertexShader( MY_SHADER_ARGS( g_pMikuModel_VS) );
	m_OpaquePSO.SetPixelShader( MY_SHADER_ARGS( g_pModelViewerPS ) );
	m_OpaquePSO.Finalize();

	m_BlendPSO = m_OpaquePSO;
	m_BlendPSO.SetRasterizerState( RasterizerDefault );
	m_BlendPSO.SetBlendState( BlendTraditional );
	m_BlendPSO.Finalize();

    m_OpaqueSkinPSO = m_OpaquePSO;
	m_OpaqueSkinPSO.SetVertexShader( MY_SHADER_ARGS( g_pMikuModel_Skin_VS) );
	m_OpaqueSkinPSO.Finalize();

    m_BlendSkinPSO = m_BlendPSO;
	m_BlendSkinPSO.SetVertexShader( MY_SHADER_ARGS( g_pMikuModel_Skin_VS) );
	m_BlendSkinPSO.Finalize();

	m_pCameraController = new MikuCameraController(m_Camera, Vector3(kYUnitVector));
	m_pCameraController->SetMotion( &m_Motion );

	m_Buffer.Create();

	g_SceneColorBuffer.SetClearColor( Color( DirectX::Colors::CornflowerBlue ).FromSRGB() );
	g_SceneDepthBuffer.SetClearDepth( m_Camera.GetClearDepth() );

	// Default values in MMD. Due to RH coord z is inverted.
	DirectionalLight mainDefault = {};
	mainDefault.Direction = XMFLOAT3( -0.5f, -1.0f, -0.5f );
	mainDefault.Color = XMFLOAT3( 154.f / 255, 154.f / 255, 154.f / 255 );

	//
	// In RH coord,
	//
	// (-1.0, -1.0,  0.0) -> shadow: -x
	// ( 0.0, -1.0,  1.0) -> shadow: +z
	// ( 0.0, -1.0, -1.0) -> shadow: -z 
	// ( 0,0,  1.0, -1.0) -> shadow: none

	m_Lights.push_back( mainDefault );

    MotionBlur::Enable = true;
    TemporalEffects::EnableTAA = false;
    FXAA::Enable = true;
    // PostEffects::EnableHDR = true;
    // PostEffects::EnableAdaptation = true;
    // SSAO::Enable = true;
}

void MikuViewer::Cleanup( void )
{
	m_DepthPSO.Destroy(); 
	m_OpaquePSO.Destroy();
	m_BlendPSO.Destroy();
	m_OpaqueSkinPSO.Destroy();
	m_BlendSkinPSO.Destroy();

	m_Buffer.Destory();
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

void MikuViewer::Update( float deltaT )
{
    ScopedTimer _prof( L"Update" );

	if (GameInput::IsFirstPressed(GameInput::kLShoulder))
		DebugZoom.Decrement();
	else if (GameInput::IsFirstPressed(GameInput::kRShoulder))
		DebugZoom.Increment();

	m_LightConstants.NumLight = static_cast<uint32_t>(m_Lights.size());
	ASSERT(m_Lights.size() <= LightsConstants::kMaxLight );
	for (auto i = 0; i < m_Lights.size(); i++) 
	{
		DirectionalLight light;
		light.Color = m_Lights[i].Color;
		XMStoreFloat3( &light.Direction, m_CameraData.View.Get3x3() * Vector3( m_Lights[i].Direction ) );
		m_LightConstants.light[i] = light;
	}

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

	m_pCameraController->Update( deltaT );

	m_CameraData.View = m_Camera.GetViewMatrix();
	m_CameraData.Projection = m_Camera.GetProjMatrix();
	m_Buffer.Update(m_CameraData);
}

void MikuViewer::RenderScene( void )
{
	GraphicsContext& gfxContext = GraphicsContext::Begin( L"Scene Render" );

    uint32_t FrameIndex = TemporalEffects::GetFrameIndexMod2();
    (FrameIndex);

	gfxContext.ClearColor( g_SceneColorBuffer );
	gfxContext.ClearDepth( g_SceneDepthBuffer );
	gfxContext.SetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	gfxContext.SetDynamicConstantBufferView( 0, m_Buffer, { kBindVertex } );
	gfxContext.SetDynamicConstantBufferView( 1, sizeof(m_LightConstants), &m_LightConstants, { kBindPixel } );
	gfxContext.SetViewportAndScissor( m_MainViewport, m_MainScissor );
	gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );

    for (auto& model : m_Models)
    {
        gfxContext.SetPipelineState( m_OpaqueSkinPSO );
        model->Draw( gfxContext, kOpaque );
        model->DrawBone( gfxContext );
        gfxContext.SetPipelineState( m_BlendSkinPSO );
        model->Draw( gfxContext, kTransparent  );
    }

    gfxContext.SetRenderTarget( nullptr );
    TemporalEffects::ResolveImage(gfxContext);

	gfxContext.Finish();
}

void MikuViewer::RenderUI( GraphicsContext& Context )
{
	auto pos = m_Camera.GetPosition();
	auto x = (float)g_SceneColorBuffer.GetWidth() - 400.f;
    float px = pos.GetX(), py = pos.GetY(), pz = pos.GetZ();

	TextContext Text(Context);
	Text.Begin();
	Text.ResetCursor( x, 10.f );
    Text.DrawFormattedString( "Position: (%.1f, %.1f, %.1f)\n", px, py, pz );
	Text.End();
}

