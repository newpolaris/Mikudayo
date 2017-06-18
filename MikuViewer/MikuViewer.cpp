#include "GameCore.h"
#include "GraphicsCore.h"
#include "PipelineState.h"
#include "CommandContext.h"
#include "GpuBuffer.h"
#include "InputLayout.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "ConstantBuffer.h"
#include "TextureManager.h"
#include "BufferManager.h"
#include "MotionBlur.h"
#include "DepthOfField.h"
#include "Camera.h"
#include "Camera1.h"
#include "CameraController.h"
#include "GameInput.h"

#include "CompiledShaders/ModelViewerVS.h"
#include "CompiledShaders/ModelViewerPS.h"

#include "DirectXColors.h"
#include <sstream>

#include "MikuModel.h"

using namespace DirectX;
using namespace GameCore;
using namespace Graphics;
using namespace Math;

// #define CAM1
const bool bRightHand = true;

// Constant buffer used to send MVP matrices to the vertex shader.
struct MVPConstants
{
	Matrix4 model;
	Matrix4 view;
	Matrix4 projection;
};

struct DirectionalLight
{
	XMFLOAT3 Color;
	float pad;
	XMFLOAT3 Direction; // incident, I
};

struct LightsConstants
{
	enum { kMaxLight = 4 };
	DirectionalLight light[kMaxLight];
	uint32_t NumLight;
};

class MikuViewer : public GameCore::IGameApp
{
public:
	MikuViewer() : m_Model( bRightHand ), m_Stage( bRightHand )
	{
	}

	virtual void Startup( void ) override;
	virtual void Cleanup( void ) override;

	virtual void Update( float deltaT ) override;
	virtual void RenderScene( void ) override;

private:

	Camera m_Camera;
	CameraController* m_pCameraController;

	MVPConstants m_MVPBufferData;
	ConstantBuffer<MVPConstants> m_Buffer;
	std::vector<DirectionalLight> m_Lights;
	LightsConstants m_LightConstants;

	D3D11_VIEWPORT m_MainViewport;
	float m_JitterDelta[2];
	D3D11_RECT m_MainScissor;

	Graphics::MikuModel m_Model;
	Graphics::MikuModel m_Stage;

	GraphicsPSO m_DepthPSO; 
	GraphicsPSO m_ModelPSO;
};

CREATE_APPLICATION( MikuViewer )

Camera1 mCam;

void MikuViewer::Startup( void )
{
	TextureManager::Initialize( L"Textures" );

	const std::wstring modelPath = L"Models/gumi3.pmd";
	const std::wstring motionPath = L"Models/gum2.vmd";
	const std::wstring stagePath = L"Models/Library.pmd";

	m_Model.LoadModel( modelPath );
	// m_Model.LoadMotion( motionPath );
	m_Model.LoadBone();
	m_Stage.LoadModel( stagePath );

	// Depth-only (2x rate)
#ifdef CAM1
	m_DepthPSO.SetRasterizerState( RasterizerDefaultCW );
#else
	m_DepthPSO.SetRasterizerState( RasterizerDefault );
#endif
	// m_DepthPSO.SetBlendState( BlendNoColorWrite );
	m_DepthPSO.SetInputLayout( static_cast<UINT>(m_Model.m_InputDesc.size()), m_Model.m_InputDesc.data() );
	m_DepthPSO.SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_ModelPSO = m_DepthPSO;
	// m_ModelPSO.SetBlendState(BlendDisable);
	// m_ModelPSO.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
#ifdef CAM1
	m_ModelPSO.SetDepthStencilState( DepthStateTestLess );
#else
	if (m_Camera.GetReverseZ())
		m_ModelPSO.SetDepthStencilState( DepthStateReadWrite );
	else
		m_ModelPSO.SetDepthStencilState( DepthStateTestLess );
#endif
	m_ModelPSO.SetVertexShader( MY_SHADER_ARGS( g_pModelViewerVS ) );
	m_ModelPSO.SetPixelShader( MY_SHADER_ARGS( g_pModelViewerPS ) );
	m_ModelPSO.Finalize();

	const Vector3 eye = Vector3( -1.0f, 15.0f, 22.0f );
	const Vector3 at = Vector3( 0.0f, 15.0f, 0.0f );

	m_Camera.SetEyeAtUp( eye, at, Vector3( kYUnitVector ) );
	m_Camera.SetZRange( 1.0f, 1000.0f );
	m_pCameraController = new CameraController( m_Camera, Vector3( kYUnitVector ) );

	XMFLOAT3 eye1(0.0, 0.0, -50);
	XMFLOAT3 target1(0.0, 0.0, 0.0);
	XMFLOAT3 up1(0.f, 1.0, 0.0);
	mCam.LookAt(eye1, target1, up1);
	mCam.Zoom(20.0f);

	m_Buffer.Create();

	g_SceneColorBuffer.SetClearColor( Color( DirectX::Colors::CornflowerBlue ).FromSRGB() );
#ifdef CAM1
	g_SceneDepthBuffer.SetClearDepth( 1.0f );
#else
	g_SceneDepthBuffer.SetClearDepth( m_Camera.GetClearDepth() );
#endif

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
}

void MikuViewer::Cleanup( void )
{
	m_DepthPSO.Destroy(); 
	m_ModelPSO.Destroy();

	m_Buffer.Destory();

	m_Model.Clear();
	m_Stage.Clear();

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
	if (GetAsyncKeyState('W') & 0x8000)
		mCam.Walk(1.0f*deltaT);

	if (GetAsyncKeyState('S') & 0x8000)
		mCam.Walk(-1.0f*deltaT);

	if (GetAsyncKeyState('A') & 0x8000)
		mCam.Strafe(-1.0f*deltaT);

	if (GetAsyncKeyState('D') & 0x8000)
		mCam.Strafe(1.0f*deltaT);

	if (GameInput::IsFirstPressed(GameInput::kLShoulder))
		DebugZoom.Decrement();
	else if (GameInput::IsFirstPressed(GameInput::kRShoulder))
		DebugZoom.Increment();

	m_pCameraController->Update(deltaT);

	auto pos = m_Camera.GetPosition();

	std::wostringstream outs;
	outs.precision( 6 );
	outs << "POS " << pos.GetX() << L" " << pos.GetY() << L" " << pos.GetZ();
	SetWindowText( GameCore::g_hWnd, outs.str().c_str());

	m_MVPBufferData.model = Matrix4( kIdentity );
#ifndef CAM1
	m_MVPBufferData.view = m_Camera.GetViewMatrix();
	m_MVPBufferData.projection = m_Camera.GetProjMatrix();
#else
	mCam.UpdateViewMatrix();
	m_MVPBufferData.view = (Matrix4)mCam.View();
	m_MVPBufferData.projection = (Matrix4)mCam.Proj();
#endif
	
	m_Buffer.Update(m_MVPBufferData);

	m_LightConstants.NumLight = static_cast<uint32_t>(m_Lights.size());
	ASSERT(m_Lights.size() <= LightsConstants::kMaxLight );
	for (auto i = 0; i < m_Lights.size(); i++) 
	{
		DirectionalLight light;
		light.Color = m_Lights[i].Color;
		XMStoreFloat3( &light.Direction, m_MVPBufferData.view.Get3x3() * Vector3( m_Lights[i].Direction ) );
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
	m_JitterDelta[0] = m_MainViewport.TopLeftX;
	m_JitterDelta[1] = m_MainViewport.TopLeftY;

	uint64_t FrameIndex = Graphics::GetFrameCount();

	if (TemporalAA::Enable && !DepthOfField::Enable)
	{
		static const float Halton23[8][2] =
		{
			{ 0.0f / 8.0f, 0.0f / 9.0f }, { 4.0f / 8.0f, 3.0f / 9.0f },
			{ 2.0f / 8.0f, 6.0f / 9.0f }, { 6.0f / 8.0f, 1.0f / 9.0f },
			{ 1.0f / 8.0f, 4.0f / 9.0f }, { 5.0f / 8.0f, 7.0f / 9.0f },
			{ 3.0f / 8.0f, 2.0f / 9.0f }, { 7.0f / 8.0f, 5.0f / 9.0f }
		};

		const float* Offset = nullptr;

		Offset = Halton23[FrameIndex % 8];

		m_MainViewport.TopLeftX = Offset[0];
		m_MainViewport.TopLeftY = Offset[1];
	}
	else
	{
		m_MainViewport.TopLeftX = 0.5f;
		m_MainViewport.TopLeftY = 0.5f;
	}

	m_JitterDelta[0] -= m_MainViewport.TopLeftX;
	m_JitterDelta[1] -= m_MainViewport.TopLeftY;

	m_MainViewport.Width = (float)g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height = (float)g_SceneColorBuffer.GetHeight();
	m_MainViewport.MinDepth = 0.0f;
	m_MainViewport.MaxDepth = 1.0f;

	m_MainScissor.left = 0;
	m_MainScissor.top = 0;
	m_MainScissor.right = (LONG)g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom = (LONG)g_SceneColorBuffer.GetHeight();

	m_Model.Update( deltaT );
	m_Model.UpdateBone( deltaT );
}

void MikuViewer::RenderScene( void )
{
	GraphicsContext& gfxContext = GraphicsContext::Begin( L"Scene Render" );
	gfxContext.ClearColor( g_SceneColorBuffer );
	gfxContext.ClearDepth( g_SceneDepthBuffer );
	gfxContext.SetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	gfxContext.SetPipelineState( m_ModelPSO );
	gfxContext.SetDynamicConstantBufferView( 0, m_Buffer, { kBindVertex } );
	gfxContext.SetDynamicConstantBufferView( 1, sizeof(m_LightConstants), &m_LightConstants, { kBindPixel } );
	gfxContext.SetViewportAndScissor( m_MainViewport, m_MainScissor );
	gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
	m_Stage.Draw( gfxContext );
	m_Model.Draw( gfxContext );

	// m_Model.DrawBone( gfxContext );

	gfxContext.Flush();
	gfxContext.Finish();
}

POINT mLastMousePos;

namespace GameCore
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	extern HWND g_hWnd;
#else
	extern Platform::Agile<Windows::UI::Core::CoreWindow> g_window;
#endif
}

void OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(g_hWnd);
}

void OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void OnMouseMove(WPARAM btnState, int x, int y)
{
	if( (btnState & MK_LBUTTON) != 0 )
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mCam.Pitch(dy);
		mCam.RotateY(dx);
	}
	else if( (btnState & MK_RBUTTON) != 0 )
	{
		// Make each pixel correspond to 0.01 unit in the scene.
		float dx = 0.01f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.01f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		// mRadius += dx - dy;

		// Restrict the radius.
		// mRadius = MathHelper::Clamp(mRadius, 1.0f, 20000.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

