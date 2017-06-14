#include "GameCore.h"
#include "GraphicsCore.h"
#include "PipelineState.h"
#include "CommandContext.h"
#include "GpuBuffer.h"
#include "InputLayout.h"
#include "ColorBuffer.h"
#include "MotionBlur.h"
#include "DepthOfField.h"
#include "BufferManager.h"

#include "CompiledShaders/ModelViewerVS.h"
#include "CompiledShaders/ModelViewerPS.h"

#include "DirectXColors.h"
#include "ConstantBuffer.h"

using namespace DirectX;
using namespace GameCore;
using namespace Graphics;
using namespace Math;

class Model
{
public:
	Model();
	~Model();

	bool Load();
	void Clear();

	VertexBuffer m_VertexBuffer;
	IndexBuffer m_IndexBuffer;
	UINT m_indexCount;
};

Model::Model()
{
}

Model::~Model()
{
	Clear();
}

bool Model::Load()
{
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};

	// Load mesh vertices. Each vertex has a position and a color.
	static const VertexPositionColor cubeVertices[] =
	{
		{XMFLOAT3( -0.5f, -0.5f, -0.5f ), XMFLOAT3( 0.0f, 0.0f, 0.0f )},
		{XMFLOAT3( -0.5f, -0.5f,  0.5f ), XMFLOAT3( 0.0f, 0.0f, 1.0f )},
		{XMFLOAT3( -0.5f,  0.5f, -0.5f ), XMFLOAT3( 0.0f, 1.0f, 0.0f )},
		{XMFLOAT3( -0.5f,  0.5f,  0.5f ), XMFLOAT3( 0.0f, 1.0f, 1.0f )},
		{XMFLOAT3( 0.5f, -0.5f, -0.5f ), XMFLOAT3( 1.0f, 0.0f, 0.0f )},
		{XMFLOAT3( 0.5f, -0.5f,  0.5f ), XMFLOAT3( 1.0f, 0.0f, 1.0f )},
		{XMFLOAT3( 0.5f,  0.5f, -0.5f ), XMFLOAT3( 1.0f, 1.0f, 0.0f )},
		{XMFLOAT3( 0.5f,  0.5f,  0.5f ), XMFLOAT3( 1.0f, 1.0f, 1.0f )},
	};

	m_VertexBuffer.Create( L"VertexBuffer", _countof(cubeVertices), sizeof(cubeVertices[0]), cubeVertices );

	// Load mesh indices. Each trio of indices represents
	// a triangle to be rendered on the screen.
	// For example: 0,2,1 means that the vertices with indexes
	// 0, 2 and 1 from the vertex buffer compose the 
	// first triangle of this mesh.
	static const unsigned short cubeIndices[] =
	{
		0,2,1, // -x
		1,2,3,

		4,5,6, // +x
		5,7,6,

		0,1,5, // -y
		0,5,4,

		2,6,7, // +y
		2,7,3,

		0,4,6, // -z
		0,6,2,

		1,3,7, // +z
		1,7,5,
	};

	m_indexCount = _countof( cubeIndices );
	m_IndexBuffer.Create( L"IndexBuffer", m_indexCount, sizeof(cubeIndices[0]), cubeIndices );

	return true;
}

void Model::Clear()
{
	m_VertexBuffer.Destroy();
	m_IndexBuffer.Destroy();
}

// Constant buffer used to send MVP matrices to the vertex shader.
__declspec(align(16)) struct MVPConstantBuffer
{
	DirectX::XMFLOAT4X4 model;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 projection;
};

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

private:
	MVPConstantBuffer m_MVPBufferData;

	D3D11_VIEWPORT m_MainViewport;
	float m_JitterDelta[2];
	D3D11_RECT m_MainScissor;

	Model m_Model;

	GraphicsPSO m_DepthPSO; 
	GraphicsPSO m_ModelPSO;
};

CREATE_APPLICATION( ModelViewer )

void ModelViewer::Startup( void )
{
	const InputDesc vertElem[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Depth-only (2x rate)
	m_DepthPSO.SetInputLayout( _countof( vertElem ), vertElem );
	m_DepthPSO.SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	m_ModelPSO = m_DepthPSO;
	m_ModelPSO.SetDepthStencilState(DepthStateReadWrite);
	m_ModelPSO.SetVertexShader( MY_SHADER_ARGS( g_pModelViewerVS ) );
	m_ModelPSO.SetPixelShader( MY_SHADER_ARGS( g_pModelViewerPS ) );
	m_ModelPSO.Finalize();

	ASSERT(m_Model.Load());

	// Copy from microsoft uwp default project
	float aspectRatio = (float)g_SceneColorBuffer.GetWidth() /  g_SceneColorBuffer.GetHeight();
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.1f,
		100.0f
		);

	XMFLOAT4X4 orientation = XMFLOAT4X4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_MVPBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
		);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.

	static const XMVECTORF32 eye = { 0.0f, 0.7f, 1.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&m_MVPBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));

	g_SceneColorBuffer.SetClearColor( DirectX::Colors::CornflowerBlue );
}

void ModelViewer::Cleanup( void )
{
	m_DepthPSO.Destroy(); 
	m_ModelPSO.Destroy();

	m_Model.Clear();
}

void ModelViewer::Update( float deltaT )
{
	// Convert degrees to radians, then convert seconds to rotation angle
	float radiansPerSecond = XMConvertToRadians( 45 );
	double totalRotation = deltaT * radiansPerSecond;
	float radians = static_cast<float>(fmod( totalRotation, XM_2PI ));

	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&m_MVPBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));

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
}

void ModelViewer::RenderScene( void )
{
	GraphicsContext& gfxContext = GraphicsContext::Begin( L"Scene Render" );
	gfxContext.ClearColor( g_SceneColorBuffer );
	gfxContext.SetVertexBuffer( 0, m_Model.m_VertexBuffer.VertexBufferView() );
	gfxContext.SetIndexBuffer( m_Model.m_IndexBuffer.IndexBufferView() );
	gfxContext.SetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	gfxContext.SetPipelineState( m_ModelPSO );
	gfxContext.SetDynamicConstantBufferView( 0, sizeof(m_MVPBufferData), &m_MVPBufferData, { kBindVertex, kBindPixel } );
	gfxContext.SetViewportAndScissor( m_MainViewport, m_MainScissor );
	D3D11_RTV_HANDLE RTVs[] = { g_SceneColorBuffer.GetRTV() };
	gfxContext.SetRenderTargets( _countof( RTVs ), RTVs );
	gfxContext.DrawIndexed( 36, 0, 0 );
	gfxContext.Flush();
	gfxContext.Finish();
}

void OnMouseDown(WPARAM , int, int) {}
void OnMouseUp(WPARAM , int, int ) { }
void OnMouseMove(WPARAM , int, int ) {}