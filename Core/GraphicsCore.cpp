//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "pch.h"
#include "Shader.h"
#include "BlendState.h"
#include "DepthStencilState.h"
#include "RasterizerState.h"
#include "InputLayout.h"
#include "GraphicsCore.h"
#include "GameCore.h"
#include "BufferManager.h"
#include "PostEffects.h"
#include "SSAO.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "CommandContext.h"
#include "CommandListManager.h"
#include "TextureManager.h"
#include "SamplerManager.h"
#include "GpuTimeManager.h"
#include "SystemTime.h"
#include "TemporalEffects.h"
#include "SSAO.h"
#include "DebugHelper.h"

// This macro determines whether to detect if there is an HDR display and enable HDR10 output.
// Currently, with HDR display enabled, the pixel magnfication functionality is broken.
#define CONDITIONALLY_ENABLE_HDR_OUTPUT 1

// Uncomment this to enable experimental support for the new shader compiler, DXC.exe
//#define DXIL

#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    #include <agile.h>
#endif

#if defined(NTDDI_WIN10_RS2) && (NTDDI_VERSION >= NTDDI_WIN10_RS2)
    #include <dxgi1_6.h>
#else
    #include <dxgi1_4.h>	// For WARP
#endif
#include <winreg.h>		// To read the registry

#include "CompiledShaders/ScreenQuadVS.h"
#include "CompiledShaders/BufferCopyPS.h"
#include "CompiledShaders/ConvertLDRToDisplayPS.h"
#include "CompiledShaders/SharpeningUpsamplePS.h"

#include "DirectXColors.h"

#define SWAP_CHAIN_BUFFER_COUNT 3

DXGI_FORMAT SwapChainFormat = DXGI_FORMAT_R10G10B10A2_UNORM;

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if ( x != nullptr ) { x->Release(); x = nullptr; }
#endif

using namespace Math;

namespace GameCore
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	extern HWND g_hWnd;
#else
	extern Platform::Agile<Windows::UI::Core::CoreWindow> g_window;
#endif
}

namespace
{
	float s_FrameTime = 0.0f;
	uint64_t s_FrameIndex = 0;
	int64_t s_FrameStartTick = 0;

	BoolVar s_EnableVSync( "Timing/VSync", true );
	BoolVar s_LimitTo30Hz( "Timing/Limit To 30Hz", false );
	BoolVar s_DropRandomFrames( "Timing/Drop Random Frames", false );
}

namespace Graphics
{
    void PreparePresentLDR();
    void PreparePresentHDR();
    void CompositeOverlays( GraphicsContext& Context );

#ifndef RELEASE
    const GUID WKPDID_D3DDebugObjectName = { 0x429b8c22,0x9188,0x4b0c, { 0x87,0x42,0xac,0xb0,0xbf,0x85,0xc2,0x00 }};
#endif

    enum eResolution { k720p, k900p, k1080p, k1440p, k1800p, k2160p };

    const uint32_t kMaxNativeWidth = 3840;
    const uint32_t kMaxNativeHeight = 2160;
    const uint32_t kNumPredefinedResolutions = 6;

    const char* ResolutionLabels[] = { "1280x720", "1600x900", "1920x1080", "2560x1440", "3200x1800", "3840x2160" };
    EnumVar TargetResolution("Graphics/Display/Native Resolution", k1080p, kNumPredefinedResolutions, ResolutionLabels);

    bool g_bTypedUAVLoadSupport_R11G11B10_FLOAT = false;
    bool g_bTypedUAVLoadSupport_R16G16B16A16_FLOAT = false;
    bool g_bEnableHDROutput = false;
    NumVar g_HDRPaperWhite("Graphics/Display/Paper White (nits)", 200.0f, 100.0f, 500.0f, 50.0f);
    NumVar g_MaxDisplayLuminance("Graphics/Display/Peak Brightness (nits)", 1000.0f, 500.0f, 10000.0f, 100.0f);
    const char* HDRModeLabels[] = { "HDR", "SDR", "Side-by-Side" };
    EnumVar HDRDebugMode("Graphics/Display/HDR Debug Mode", 0, 3, HDRModeLabels);

    uint32_t g_NativeWidth = 0;
    uint32_t g_NativeHeight = 0;
    uint32_t g_DisplayWidth = 1920;
    uint32_t g_DisplayHeight = 1080;
    ColorBuffer g_PreDisplayBuffer;

    void SetNativeResolution(void)
    {
        uint32_t NativeWidth, NativeHeight;

        switch (eResolution((int)TargetResolution))
        {
        default:
        case k720p:
            NativeWidth = 1280;
            NativeHeight = 720;
            break;
        case k900p:
            NativeWidth = 1600;
            NativeHeight = 900;
            break;
        case k1080p:
            NativeWidth = 1920;
            NativeHeight = 1080;
            break;
        case k1440p:
            NativeWidth = 2560;
            NativeHeight = 1440;
            break;
        case k1800p:
            NativeWidth = 3200;
            NativeHeight = 1800;
            break;
        case k2160p:
            NativeWidth = 3840;
            NativeHeight = 2160;
            break;
        }

		if (g_NativeWidth == NativeWidth && g_NativeHeight == NativeHeight)
			return;

		DEBUGPRINT("Changing native resolution to %ux%u", NativeWidth, NativeHeight);

		g_NativeWidth = NativeWidth;
		g_NativeHeight = NativeHeight;

		g_CommandManager.IdleGPU();

		InitializeRenderingBuffers( NativeWidth, NativeHeight );
	}

	ID3D11_DEVICE* g_Device = nullptr;

	CommandListManager g_CommandManager;
	ContextManager g_ContextManager;

	D3D_FEATURE_LEVEL g_D3DFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	ColorBuffer g_DisplayPlane;

	IDXGISwapChain1* s_SwapChain1 = nullptr;
	ID3D11_CONTEXT* g_Context = nullptr;

	SamplerDesc SamplerLinearWrapDesc;
	SamplerDesc SamplerAnisoWrapDesc;
	SamplerDesc SamplerShadowDescGE;
	SamplerDesc SamplerShadowDescLE;
	SamplerDesc SamplerLinearClampDesc;
	SamplerDesc SamplerVolumeWrapDesc;
	SamplerDesc SamplerPointClampDesc;
	SamplerDesc SamplerPointBorderDesc;
	SamplerDesc SamplerLinearBorderDesc;

	D3D11_SAMPLER_HANDLE SamplerLinearWrap;
	D3D11_SAMPLER_HANDLE SamplerAnisoWrap;
	D3D11_SAMPLER_HANDLE SamplerShadowGE;
	D3D11_SAMPLER_HANDLE SamplerShadowLE;
	D3D11_SAMPLER_HANDLE SamplerShadow;
	D3D11_SAMPLER_HANDLE SamplerLinearClamp;
	D3D11_SAMPLER_HANDLE SamplerVolumeWrap;
	D3D11_SAMPLER_HANDLE SamplerPointClamp;
	D3D11_SAMPLER_HANDLE SamplerPointBorder;
	D3D11_SAMPLER_HANDLE SamplerLinearBorder;

	D3D11_RASTERIZER_DESC RasterizerDefault;
	D3D11_RASTERIZER_DESC RasterizerDefaultCW;
	D3D11_RASTERIZER_DESC RasterizerTwoSided;
	D3D11_RASTERIZER_DESC RasterizerWireframe;
	D3D11_RASTERIZER_DESC RasterizerShadow;
	D3D11_RASTERIZER_DESC RasterizerShadowCW;
	D3D11_RASTERIZER_DESC RasterizerShadowTwoSided;

	D3D11_BLEND_DESC BlendNoColorWrite;
	D3D11_BLEND_DESC BlendDisable;
	D3D11_BLEND_DESC BlendPreMultiplied;
	D3D11_BLEND_DESC BlendTraditional;
	D3D11_BLEND_DESC BlendAdditive;
	D3D11_BLEND_DESC BlendTraditionalAdditive;

	D3D11_DEPTH_STENCIL_DESC DepthStateDisabled;
	D3D11_DEPTH_STENCIL_DESC DepthStateReadWrite;
	D3D11_DEPTH_STENCIL_DESC DepthStateReadOnly;
	D3D11_DEPTH_STENCIL_DESC DepthStateReadOnlyReversed;
	D3D11_DEPTH_STENCIL_DESC DepthStateTestEqual;

    GraphicsPSO s_BlendUIPSO;
    GraphicsPSO PresentSDRPS;
    GraphicsPSO PresentHDRPS;
	GraphicsPSO ConvertLDRToDisplayPS;
	GraphicsPSO ConvertHDRToDisplayPS;
	GraphicsPSO MagnifyPixelsPS;
	GraphicsPSO SharpeningUpsamplePS;
	GraphicsPSO BicubicHorizontalUpsamplePS;
	GraphicsPSO BicubicVerticalUpsamplePS;
	GraphicsPSO BilinearUpsamplePS;

	ComputePSO g_GenerateMipsLinearPSO[4];
	ComputePSO g_GenerateMipsGammaPSO[4];

	enum { kBilinear, kBicubic, kSharpening, kFilterCount };
	const char* FilterLabels[] = { "Bilinear", "Bicubic", "Sharpening" };
	EnumVar UpsampleFilter("Graphics/Display/Upsample Filter", kFilterCount - 1, kFilterCount, FilterLabels);
	NumVar BicubicUpsampleWeight("Graphics/Display/Bicubic Filter Weight", -0.75f, -1.0f, -0.25f, 0.25f);
	NumVar SharpeningSpread("Graphics/Display/Sharpness Sample Spread", 1.0f, 0.7f, 2.0f, 0.1f);
    NumVar SharpeningRotation("Graphics/Display/Sharpness Sample Rotation", 45.0f, 0.0f, 90.0f, 15.0f);
    NumVar SharpeningStrength("Graphics/Display/Sharpness Strength", 0.10f, 0.0f, 1.0f, 0.01f);

    enum DebugZoomLevel { kDebugZoomOff, kDebugZoom2x, kDebugZoom4x, kDebugZoom8x, kDebugZoom16x, kDebugZoomCount };
    const char* DebugZoomLabels[] = { "Off", "2x Zoom", "4x Zoom", "8x Zoom", "16x Zoom" };
    EnumVar DebugZoom("Graphics/Display/Magnify Pixels", kDebugZoomOff, kDebugZoomCount, DebugZoomLabels);
}

void Graphics::Resize(uint32_t width, uint32_t height)
{
    ASSERT(s_SwapChain1 != nullptr);

    g_CommandManager.IdleGPU();

    g_DisplayWidth = width;
    g_DisplayHeight = height;

    DEBUGPRINT("Changing display resolution to %ux%u", width, height);

    g_PreDisplayBuffer.Create(L"PreDisplay Buffer", width, height, 1, SwapChainFormat);

	g_DisplayPlane.Destroy();

	ASSERT_SUCCEEDED(s_SwapChain1->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, width, height, SwapChainFormat, 0));

	ComPtr<ID3D11Texture2D> BackBuffer;
	ASSERT_SUCCEEDED(s_SwapChain1->GetBuffer(0, MY_IID_PPV_ARGS(&BackBuffer)));
	g_DisplayPlane.CreateFromSwapChain(L"Primary SwapChain Buffer", BackBuffer);


    g_CommandManager.IdleGPU();
    ResizeDisplayDependentBuffers(g_NativeWidth, g_NativeHeight);
}

// Initialize the DirectX resources required to run.
void Graphics::Initialize( void )
{
	ASSERT(s_SwapChain1 == nullptr, "Graphics has already been initialized");

	ComPtr<ID3D11Device> pDevice;
	ComPtr<ID3D11DeviceContext> pContext;

#if _DEBUG
	Microsoft::WRL::ComPtr<ID3D11Debug> debugnterface;

#endif

	// Obtain the DXGI factory
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	ASSERT_SUCCEEDED(CreateDXGIFactory2(0, MY_IID_PPV_ARGS(&dxgiFactory)));

	// Create the D3D graphics device
	Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;

	static const bool bUseWarpDriver = false;

	// This flag adds support for surfaces with a different color channel ordering
	// than the API default. It is required for compatibility with Direct2D.
	UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] = 
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	if (!bUseWarpDriver)
	{
		SIZE_T MaxSize = 0;
		for (uint32_t Idx = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(Idx, &pAdapter); ++Idx)
		{
			DXGI_ADAPTER_DESC1 desc1;
			pAdapter->GetDesc1(&desc1);
			if (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;

			if (desc1.DedicatedVideoMemory > MaxSize && 
				SUCCEEDED(D3D11CreateDevice(
					pAdapter.Get(), 
					D3D_DRIVER_TYPE_UNKNOWN, 
					nullptr,
					deviceFlags,
					featureLevels,
					_countof(featureLevels),
					D3D11_SDK_VERSION,
					pDevice.ReleaseAndGetAddressOf(),
					&g_D3DFeatureLevel,
					pContext.ReleaseAndGetAddressOf() ) ))

			{
				Utility::Printf(L"D3D11-capable hardware found:  %s (%u MB)\n", desc1.Description, desc1.DedicatedVideoMemory >> 20);
				MaxSize = desc1.DedicatedVideoMemory;
			}
		}

		if (MaxSize > 0) {
			ComPtr<ID3D11_DEVICE> device;
			SUCCEEDED(pDevice.As(&device));
			g_Device = device.Detach();

			ComPtr<ID3D11_CONTEXT> context;
			SUCCEEDED( pContext.As( &context ) );
			g_Context = context.Detach();
		}
	}

	if (g_Device == nullptr)
	{
		if (bUseWarpDriver)
			Utility::Print("WARP software adapter requested. Initializing...\n");
		else
			Utility::Print("Failed to find a hardware adapter.  Falling back to WARP.\n");

		ASSERT_SUCCEEDED( dxgiFactory->EnumWarpAdapter( IID_PPV_ARGS( &pAdapter ) ) );
		ASSERT_SUCCEEDED( D3D11CreateDevice(
			pAdapter.Get(),
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			deviceFlags,
			featureLevels,
			ARRAYSIZE( featureLevels ),
			D3D11_SDK_VERSION,
			pDevice.ReleaseAndGetAddressOf(),
			&g_D3DFeatureLevel,
			pContext.ReleaseAndGetAddressOf() ) );

		ComPtr<ID3D11_DEVICE> device;
		SUCCEEDED( pDevice.As( &device ) );
		g_Device = device.Detach();

		ComPtr<ID3D11_CONTEXT> context;
		SUCCEEDED( pContext.As( &context ) );
		g_Context = context.Detach();
	}

#ifndef RELEASE
    else
    {
        bool DeveloperModeEnabled = false;

        // Look in the Windows Registry to determine if Developer Mode is enabled
        HKEY hKey;
        LSTATUS result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, KEY_READ, &hKey);
        if (result == ERROR_SUCCESS)
        {
            DWORD keyValue, keySize = sizeof(DWORD);
            result = RegQueryValueEx(hKey, L"AllowDevelopmentWithoutDevLicense", 0, NULL, (byte*)&keyValue, &keySize);
            if (result == ERROR_SUCCESS && keyValue == 1)
                DeveloperModeEnabled = true;
            RegCloseKey(hKey);
        }

        WARN_ONCE_IF_NOT(DeveloperModeEnabled, "Enable Developer Mode on Windows 10 to get consistent profiling results");

        // Prevent the GPU from overclocking or underclocking to get consistent timings
        // if (DeveloperModeEnabled)
        //    g_Device->SetStablePowerState(TRUE);
    }
#endif	
#if _DEBUG
	ID3D11InfoQueue* pInfoQueue = nullptr;
	if (SUCCEEDED(g_Device->QueryInterface(MY_IID_PPV_ARGS(&pInfoQueue))))
	{
		// Suppress whole categories of messages
		// D3D11_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D11_MESSAGE_SEVERITY Severities[] = 
		{
			D3D11_MESSAGE_SEVERITY_INFO
		};

		D3D11_MESSAGE_ID Id[] = 
		{
			D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
		};

		D3D11_INFO_QUEUE_FILTER NewFilter = {};
		// NewFilter.DenyList.NumCategories = _countof(Categories);
		// NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(Id);
		NewFilter.DenyList.pIDList = Id;
		pInfoQueue->PushStorageFilter(&NewFilter);
		pInfoQueue->Release();
	}
#endif

	// We like to do read-modify-write operations on UAVs during post processing.  To support that, we
	// need to either have the hardware do typed UAV loads of R11G11B10_FLOAT or we need to manually
	// decode an R32_UINT representation of the same buffer.  This code determines if we get the hardware
	// load support.
	D3D11_FEATURE_DATA_D3D11_OPTIONS2 FeatureData = {};
	if (SUCCEEDED(g_Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &FeatureData, sizeof(FeatureData))))
	{
		if (FeatureData.TypedUAVLoadAdditionalFormats)
		{
			D3D11_FEATURE_DATA_FORMAT_SUPPORT Support =
			{
				DXGI_FORMAT_R11G11B10_FLOAT, 0
			};

			if (SUCCEEDED(g_Device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
				(Support.OutFormatSupport & D3D11_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
			{
				g_bTypedUAVLoadSupport_R11G11B10_FLOAT = true;
			}
		}
	}

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = g_DisplayWidth;
    swapChainDesc.Height = g_DisplayHeight;
    swapChainDesc.Format = SwapChainFormat;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	ASSERT_SUCCEEDED(dxgiFactory->CreateSwapChainForHwnd(g_Device, GameCore::g_hWnd, &swapChainDesc, nullptr, nullptr, &s_SwapChain1));
#else
	ASSERT_SUCCEEDED(dxgiFactory->CreateSwapChainForCoreWindow(g_CommandManager.GetCommandQueue(), (IUnknown*)GameCore::g_window.Get(), &swapChainDesc, nullptr, &s_SwapChain1));
#endif

#if CONDITIONALLY_ENABLE_HDR_OUTPUT && defined(NTDDI_WIN10_RS2) && (NTDDI_VERSION >= NTDDI_WIN10_RS2)
    {
        IDXGISwapChain4* swapChain = (IDXGISwapChain4*)s_SwapChain1;
        ComPtr<IDXGIOutput> output;
        ComPtr<IDXGIOutput6> output6;
        DXGI_OUTPUT_DESC1 outputDesc;
        UINT colorSpaceSupport;

        // Query support for ST.2084 on the display and set the color space accordingly
        if (SUCCEEDED(swapChain->GetContainingOutput(&output)) &&
            SUCCEEDED(output.As(&output6)) &&
            SUCCEEDED(output6->GetDesc1(&outputDesc)) &&
            outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 &&
            SUCCEEDED(swapChain->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &colorSpaceSupport)) &&
            (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) &&
            SUCCEEDED(swapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)))
        {
            g_bEnableHDROutput = true;
        }
    }
#endif


	ComPtr<ID3D11Texture2D1> BackBuffer;
	ASSERT_SUCCEEDED(s_SwapChain1->GetBuffer(0, MY_IID_PPV_ARGS(&BackBuffer)));
	g_DisplayPlane.CreateFromSwapChain(L"Primary SwapChain Buffer", BackBuffer);

	SamplerLinearWrapDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerLinearWrap = SamplerLinearWrapDesc.CreateDescriptor();

	SamplerAnisoWrapDesc.MaxAnisotropy = 4;
	SamplerAnisoWrap = SamplerAnisoWrapDesc.CreateDescriptor();

	SamplerShadowDescGE.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	SamplerShadowDescGE.ComparisonFunc = D3D11_COMPARISON_GREATER_EQUAL;
	SamplerShadowDescGE.SetTextureAddressMode(D3D11_TEXTURE_ADDRESS_CLAMP);
	SamplerShadowGE = SamplerShadowDescGE.CreateDescriptor();

    SamplerShadowDescLE = SamplerShadowDescGE;
	SamplerShadowDescLE.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	SamplerShadowLE = SamplerShadowDescLE.CreateDescriptor();

    SamplerShadow = Math::g_ReverseZ ? SamplerShadowGE : SamplerShadowLE;

	SamplerLinearClampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerLinearClampDesc.SetTextureAddressMode(D3D11_TEXTURE_ADDRESS_CLAMP);
	SamplerLinearClamp = SamplerLinearClampDesc.CreateDescriptor();

	SamplerVolumeWrapDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	SamplerVolumeWrap = SamplerVolumeWrapDesc.CreateDescriptor();

	SamplerPointClampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	SamplerPointClampDesc.SetTextureAddressMode(D3D11_TEXTURE_ADDRESS_CLAMP);
	SamplerPointClamp = SamplerPointClampDesc.CreateDescriptor();

	SamplerLinearBorderDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerLinearBorderDesc.SetTextureAddressMode(D3D11_TEXTURE_ADDRESS_BORDER);
	SamplerLinearBorderDesc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
	SamplerLinearBorder = SamplerLinearBorderDesc.CreateDescriptor();

	SamplerPointBorderDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	SamplerPointBorderDesc.SetTextureAddressMode(D3D11_TEXTURE_ADDRESS_BORDER);
	SamplerPointBorderDesc.SetBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
	SamplerPointBorder = SamplerPointBorderDesc.CreateDescriptor();

	// Default rasterizer states
	RasterizerDefault.FillMode = D3D11_FILL_SOLID;
	RasterizerDefault.CullMode = D3D11_CULL_BACK;
	RasterizerDefault.FrontCounterClockwise = TRUE;
	RasterizerDefault.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
	RasterizerDefault.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
	RasterizerDefault.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	RasterizerDefault.DepthClipEnable = TRUE;
	RasterizerDefault.MultisampleEnable = FALSE;
	RasterizerDefault.AntialiasedLineEnable = FALSE;

	RasterizerDefaultCW = RasterizerDefault;
	RasterizerDefaultCW.FrontCounterClockwise = FALSE;

	RasterizerTwoSided = RasterizerDefault;
	RasterizerTwoSided.CullMode = D3D11_CULL_NONE;

	RasterizerWireframe = RasterizerTwoSided;
	RasterizerWireframe.FillMode = D3D11_FILL_WIREFRAME;

	// Shadows need their own rasterizer state so we can reverse the winding of faces
	RasterizerShadow = RasterizerDefault;
	RasterizerShadow.CullMode = D3D11_CULL_FRONT;  // Hacked here rather than fixing the content
	RasterizerShadow.SlopeScaledDepthBias = Math::g_ReverseZ ? -1.5f : 1.5f;
	RasterizerShadow.DepthBias = Math::g_ReverseZ ? -100 : 100;

	RasterizerShadowTwoSided = RasterizerShadow;
	RasterizerShadowTwoSided.CullMode = D3D11_CULL_NONE;

	RasterizerShadowCW = RasterizerShadow;
	RasterizerShadowCW.FrontCounterClockwise = FALSE;

	DepthStateDisabled.DepthEnable = FALSE;
	DepthStateDisabled.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DepthStateDisabled.DepthFunc = D3D11_COMPARISON_ALWAYS;
	DepthStateDisabled.StencilEnable = FALSE;
	DepthStateDisabled.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	DepthStateDisabled.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	DepthStateDisabled.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	DepthStateDisabled.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DepthStateDisabled.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStateDisabled.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStateDisabled.BackFace = DepthStateDisabled.FrontFace;

	DepthStateReadWrite = DepthStateDisabled;
	DepthStateReadWrite.DepthEnable = TRUE;
	DepthStateReadWrite.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DepthStateReadWrite.DepthFunc = Math::g_ReverseZ ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;

	DepthStateReadOnly = DepthStateReadWrite;
	DepthStateReadOnly.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	DepthStateReadOnlyReversed = DepthStateReadOnly;
	DepthStateReadOnlyReversed.DepthFunc = Math::g_ReverseZ ? D3D11_COMPARISON_LESS : D3D11_COMPARISON_GREATER;

	DepthStateTestEqual = DepthStateReadOnly;
	DepthStateTestEqual.DepthFunc = D3D11_COMPARISON_EQUAL;

	D3D11_BLEND_DESC alphaBlend = {};
	alphaBlend.IndependentBlendEnable = FALSE;
	alphaBlend.RenderTarget[0].BlendEnable = FALSE;
	alphaBlend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	alphaBlend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	alphaBlend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	alphaBlend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	alphaBlend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	alphaBlend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	alphaBlend.RenderTarget[0].RenderTargetWriteMask = 0;
	BlendNoColorWrite = alphaBlend;

	alphaBlend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	BlendDisable = alphaBlend;

	alphaBlend.RenderTarget[0].BlendEnable = TRUE;
	BlendTraditional = alphaBlend;

	alphaBlend.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	BlendPreMultiplied = alphaBlend;

	alphaBlend.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	BlendAdditive = alphaBlend;

	alphaBlend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendTraditionalAdditive = alphaBlend;

	s_BlendUIPSO.SetRasterizerState( RasterizerTwoSided );
	s_BlendUIPSO.SetBlendState( BlendPreMultiplied );
	s_BlendUIPSO.SetDepthStencilState( DepthStateDisabled );
	s_BlendUIPSO.SetSampleMask( 0xFFFFFFFF );
	s_BlendUIPSO.SetInputLayout( 0, nullptr );
	s_BlendUIPSO.SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	s_BlendUIPSO.SetVertexShader( MY_SHADER_ARGS( g_pScreenQuadVS ) );
	s_BlendUIPSO.SetPixelShader( MY_SHADER_ARGS( g_pBufferCopyPS ) );
	s_BlendUIPSO.Finalize();

#define CreatePSO( ObjName, ShaderByteCode ) \
	ObjName = s_BlendUIPSO; \
	ObjName.SetBlendState( BlendDisable ); \
	ObjName.SetPixelShader( MY_SHADER_ARGS( ShaderByteCode) ); \
	ObjName.Finalize();

	CreatePSO( ConvertLDRToDisplayPS, g_pConvertLDRToDisplayPS );
	// CreatePSO( MagnifyPixelsPS, g_pMagnifyPixelsPS );
	// CreatePSO( BilinearUpsamplePS, g_pBilinearUpsamplePS );
	// CreatePSO( BicubicHorizontalUpsamplePS, g_pBicubicHorizontalUpsamplePS );
	// CreatePSO( BicubicVerticalUpsamplePS, g_pBicubicVerticalUpsamplePS );
	CreatePSO( SharpeningUpsamplePS, g_pSharpeningUpsamplePS );

#undef CreatePSO

	g_PreDisplayBuffer.Create(L"PreDisplay Buffer", g_DisplayWidth, g_DisplayHeight, 1, SwapChainFormat);

    GpuTimeManager::Initialize(1024);
	SetNativeResolution();
    TemporalEffects::Initialize();
    PostEffects::Initialize();
    // SSAO::Initialize();
    TextRenderer::Initialize();
    // GraphRenderer::Initialize();
    // ParticleEffects::Initialize(kMaxNativeWidth, kMaxNativeHeight);
    Utility::Initialize();
}

void Graphics::Terminate( void )
{
	g_CommandManager.IdleGPU();
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	s_SwapChain1->SetFullscreenState(FALSE, nullptr);
#endif
}

void Graphics::Shutdown( void )
{
	CommandContext::DestroyAllContexts();
    g_CommandManager.Shutdown();
    GpuTimeManager::Shutdown();
	ConvertLDRToDisplayPS.Destroy();
	SharpeningUpsamplePS.Destroy();

	s_BlendUIPSO.Destroy();

	Shader::DestroyAll();
	BlendState::DestroyAll();
	RasterizerState::DestroyAll();
	SamplerDesc::DestroyAll();
	InputLayout::DestroyAll();
	DepthStencilState::DestroyAll();

    DestroyRenderingBuffers();
    TemporalEffects::Shutdown();
    PostEffects::Shutdown();
    Utility::Shutdown();
    // SSAO::Shutdown();
    TextRenderer::Shutdown();
    // GraphRenderer::Shutdown();
    // ParticleEffects::Shutdown();
	TextureManager::Shutdown();

	DestroyRenderingBuffers();

	SAFE_RELEASE(g_Context);
	SAFE_RELEASE(s_SwapChain1);

	g_DisplayPlane.Destroy();
	g_PreDisplayBuffer.Destroy();

#if defined(_DEBUG)
	ID3D11Debug* debugInterface = nullptr;
	if (SUCCEEDED(g_Device->QueryInterface(MY_IID_PPV_ARGS(&debugInterface))))
	{
		debugInterface->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
		SAFE_RELEASE(debugInterface);
	}
#endif

	SAFE_RELEASE(g_Device);
}

void Graphics::PreparePresentLDR( void )
{
	GraphicsContext& Context = GraphicsContext::Begin(L"Present");
		
	// We're going to be reading these buffers to write to the swap chain buffer(s)
	Context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Copy (and convert) the LDR buffer to the back buffer
	Context.SetDynamicDescriptor( 0, g_SceneColorBuffer.GetSRV(), { kBindPixel } );
	D3D11_SAMPLER_HANDLE sampler[] = { SamplerLinearClamp, SamplerPointClamp };
	Context.SetDynamicSamplers( 0, 2, sampler, { kBindPixel } );

	ColorBuffer& UpsampleDest = (DebugZoom == kDebugZoomOff ? g_DisplayPlane : g_PreDisplayBuffer);

	if (g_NativeWidth == g_DisplayWidth && g_NativeHeight == g_DisplayHeight)
	{
		Context.SetPipelineState(ConvertLDRToDisplayPS);
		Context.SetRenderTarget(UpsampleDest.GetRTV());
		Context.SetViewportAndScissor(0, 0, g_NativeWidth, g_NativeHeight);
		Context.Draw(3);
	}
	else if (UpsampleFilter == kBicubic)
	{
		Context.SetRenderTarget(g_HorizontalBuffer.GetRTV());
		Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_NativeHeight);
		Context.SetPipelineState(BicubicHorizontalUpsamplePS);
		Context.SetConstants( 0, g_NativeWidth, g_NativeHeight, (float)BicubicUpsampleWeight, { kBindPixel, kBindVertex } );
		Context.Draw(3);

		Context.SetRenderTarget(UpsampleDest.GetRTV());
		Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
		Context.SetPipelineState(BicubicVerticalUpsamplePS);
		Context.SetConstants( 0, g_DisplayWidth, g_NativeHeight, (float)BicubicUpsampleWeight, { kBindPixel, kBindVertex } );
		Context.SetDynamicDescriptor( 0, g_HorizontalBuffer.GetSRV(), { kBindPixel } );
		Context.Draw(3);
	}
	else if (UpsampleFilter == kSharpening)
	{
		Context.SetPipelineState( SharpeningUpsamplePS );
		Context.SetRenderTarget(UpsampleDest.GetRTV());
		Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
		float TexelWidth = 1.0f / g_NativeWidth;
		float TexelHeight = 1.0f / g_NativeHeight;
		float X = Math::Cos((float)SharpeningRotation * XM_PI / 180.0f) * (float)SharpeningSpread;
		float Y = Math::Sin((float)SharpeningRotation * XM_PI / 180.0f) * (float)SharpeningSpread;
		const float WA = (float)SharpeningStrength;
		const float WB = 1.0f + 4.0f * WA;
		float Constants[] = { X * TexelWidth, Y * TexelHeight, Y * TexelWidth, -X * TexelHeight, WA, WB };
		Context.SetConstants( 0, _countof( Constants ), Constants, { kBindPixel } );
		Context.Draw(3);
	}
	else if (UpsampleFilter == kBilinear)
	{
		Context.SetPipelineState(BilinearUpsamplePS);
		Context.SetRenderTarget(UpsampleDest.GetRTV());
		Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
		Context.Draw(3);
	}

	if (DebugZoom != kDebugZoomOff)
	{
		Context.SetPipelineState(MagnifyPixelsPS);
		Context.SetRenderTarget(g_DisplayPlane.GetRTV());
		Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
		Context.SetConstants( 0, 1.0f / ((int)DebugZoom + 1.0f), { kBindVertex, kBindPixel } );
		Context.SetDynamicDescriptor(0, g_PreDisplayBuffer.GetSRV(), { kBindPixel } );
		Context.Draw(3);
	}

	// Now blend (or write) the UI overlay
	Context.SetDynamicDescriptor( 0, g_OverlayBuffer.GetSRV(), { kBindPixel } );
	Context.SetPipelineState(s_BlendUIPSO);
	Context.Draw(3);

	// Close the final context to be executed before frame present.
	Context.Finish();
}

void Graphics::PreparePresentHDR( void )
{
}

void Graphics::Present( void )
{
	if (g_bEnableHDROutput)
		PreparePresentHDR();
	else
		PreparePresentLDR();

	UINT PresentInterval = s_EnableVSync ? std::min(4, (int)Round(s_FrameTime * 60.0f)) : 0;

	s_SwapChain1->Present( PresentInterval, 0 );

	// Test robustness to handle spikes in CPU time
	//if (s_DropRandomFrames)
	//{
	//	if (std::rand() % 25 == 0)
	//		BusyLoopSleep(0.010);
	//}

	int64_t CurrentTick = SystemTime::GetCurrentTick();

    //
    // TODO: If it does not fit 60fps, s_FrameTime will not be set correctly. As a result, the animation is slowly displayed
    //
	if (s_EnableVSync)
	{
		// With VSync enabled, the time step between frames becomes a multiple of 16.666 ms.  We need
		// to add logic to vary between 1 and 2 (or 3 fields).  This delta time also determines how
		// long the previous frame should be displayed (i.e. the present interval.)
		s_FrameTime = (s_LimitTo30Hz ? 2.0f : 1.0f) / 60.0f;
		if (s_DropRandomFrames)
		{
			if (std::rand() % 50 == 0)
				s_FrameTime += (1.0f / 60.0f);
		}
	}
	else
	{
		if (s_FrameStartTick == 0)
			s_FrameStartTick = CurrentTick;

		// When running free, keep the most recent total frame time as the time step for
		// the next frame simulation. This is not super-accurate, but assuming a frame
		// time varies smoothly, it should be close enough.
		s_FrameTime = (float)SystemTime::TimeBetweenTicks( s_FrameStartTick, CurrentTick );
	}

	s_FrameStartTick = CurrentTick;

	++s_FrameIndex;
    TemporalEffects::Update((uint32_t)s_FrameIndex);

	SetNativeResolution();
}

uint64_t Graphics::GetFrameCount(void)
{
	return s_FrameIndex;
}

float Graphics::GetFrameTime(void)
{
	return s_FrameTime;
}

float Graphics::GetFrameRate(void)
{
	return s_FrameTime == 0.0f ? 0.0f : 1.0f / s_FrameTime;
}
