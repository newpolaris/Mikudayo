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
// Author(s):  Alex Nankervis
//             James Stanard
//

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
#include "Camera.h"
#include "CameraController.h"
#include "SamplerManager.h"
#include "Model.h"
#include "GameInput.h"
#include "TemporalEffects.h"
#include "SSAO.h"
#include "FXAA.h"
#include "PostEffects.h"
#include "ForwardPlusLighting.h"

#include "CompiledShaders/ModelViewerVS.h"
#include "CompiledShaders/ModelViewerPS.h"

#include "DirectXColors.h"
#include "ConstantBuffer.h"

using namespace GameCore;
using namespace Graphics;
using namespace Math;

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

    void RenderLightShadows(GraphicsContext& gfxContext);

    enum eObjectFilter { kOpaque = 0x1, kCutout = 0x2, kTransparent = 0x4, kAll = 0xF, kNone = 0x0 };
    void RenderObjects( GraphicsContext& Context, const Matrix4& ViewProjMat, eObjectFilter Filter = kAll );
    void CreateParticleEffects();
    Camera m_Camera;
    std::auto_ptr<CameraController> m_CameraController;
    Matrix4 m_ViewProjMatrix;
    D3D11_VIEWPORT m_MainViewport;
    D3D11_RECT m_MainScissor;

    GraphicsPSO m_DepthPSO;
    GraphicsPSO m_CutoutDepthPSO;
    GraphicsPSO m_ModelPSO;
#ifdef _WAVE_OP
    GraphicsPSO m_DepthWaveOpsPSO;
    GraphicsPSO m_ModelWaveOpsPSO;
#endif
    GraphicsPSO m_CutoutModelPSO;
    GraphicsPSO m_ShadowPSO;
    GraphicsPSO m_CutoutShadowPSO;
    GraphicsPSO m_WaveTileCountPSO;

    D3D11_SAMPLER_HANDLE m_DefaultSampler;

    D3D11_SRV_HANDLE m_ExtraTextures[6];
    Model m_Model;
    std::vector<bool> m_pMaterialIsCutout;

    Vector3 m_SunDirection;
    // ShadowCamera m_SunShadow;
};

CREATE_APPLICATION( ModelViewer )

ExpVar m_SunLightIntensity("Application/Lighting/Sun Light Intensity", 4.0f, 0.0f, 16.0f, 0.1f);
ExpVar m_AmbientIntensity("Application/Lighting/Ambient Intensity", 0.1f, -16.0f, 16.0f, 0.1f);
NumVar m_SunOrientation("Application/Lighting/Sun Orientation", -0.5f, -100.0f, 100.0f, 0.1f );
NumVar m_SunInclination("Application/Lighting/Sun Inclination", 0.75f, 0.0f, 1.0f, 0.01f );
NumVar ShadowDimX("Application/Lighting/Shadow Dim X", 5000, 1000, 10000, 100 );
NumVar ShadowDimY("Application/Lighting/Shadow Dim Y", 3000, 1000, 10000, 100 );
NumVar ShadowDimZ("Application/Lighting/Shadow Dim Z", 3000, 1000, 10000, 100 );

BoolVar ShowWaveTileCounts("Application/Forward+/Show Wave Tile Counts", false);
#ifdef _WAVE_OP
BoolVar EnableWaveOps("Application/Forward+/Enable Wave Ops", true);
#endif

void ModelViewer::Startup( void )
{
    SamplerDesc DefaultSamplerDesc;
    DefaultSamplerDesc.MaxAnisotropy = 8;
    m_DefaultSampler = DefaultSamplerDesc.CreateDescriptor();

    DXGI_FORMAT ColorFormat = g_SceneColorBuffer.GetFormat();
    DXGI_FORMAT DepthFormat = g_SceneDepthBuffer.GetFormat();
    // DXGI_FORMAT ShadowFormat = g_ShadowBuffer.GetFormat();

    InputDesc vertElem[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    // Depth-only (2x rate)
    m_DepthPSO.SetRasterizerState(RasterizerDefault);
    m_DepthPSO.SetBlendState(BlendNoColorWrite);
    m_DepthPSO.SetDepthStencilState(DepthStateReadWrite);
    m_DepthPSO.SetInputLayout(_countof(vertElem), vertElem);
    m_DepthPSO.SetPrimitiveTopologyType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // m_DepthPSO.SetRenderTargetFormats(0, nullptr, DepthFormat);
    // m_DepthPSO.SetVertexShader( MY_SHADER_ARGS(g_pDepthViewerVS) );
    // m_DepthPSO.Finalize();

    // Depth-only shading but with alpha testing
    m_CutoutDepthPSO = m_DepthPSO;
    // m_CutoutDepthPSO.SetPixelShader( MY_SHADER_ARGS(g_pDepthViewerPS) );
    m_CutoutDepthPSO.SetRasterizerState(RasterizerTwoSided);
    // m_CutoutDepthPSO.Finalize();

    // Depth-only but with a depth bias and/or render only backfaces
    m_ShadowPSO = m_DepthPSO;
    m_ShadowPSO.SetRasterizerState(RasterizerShadow);
    // m_ShadowPSO.Finalize();

    // Shadows with alpha testing
    m_CutoutShadowPSO = m_ShadowPSO;
    // m_CutoutShadowPSO.SetPixelShader( MY_SHADER_ARGS(g_pDepthViewerPS) );
    m_CutoutShadowPSO.SetRasterizerState(RasterizerShadowTwoSided);
    // m_CutoutShadowPSO.Finalize();

    // Full color pass
    m_ModelPSO = m_DepthPSO;
    m_ModelPSO.SetBlendState(BlendDisable);
    m_ModelPSO.SetDepthStencilState(DepthStateReadWrite);
    // m_ModelPSO.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
    m_ModelPSO.SetVertexShader( MY_SHADER_ARGS(g_pModelViewerVS) );
    m_ModelPSO.SetPixelShader( MY_SHADER_ARGS(g_pModelViewerPS) );
    m_ModelPSO.Finalize();

#ifdef _WAVE_OP
    m_DepthWaveOpsPSO = m_DepthPSO;
    m_DepthWaveOpsPSO.SetVertexShader( MY_SHADER_ARGS(g_pDepthViewerVS_SM6) );
    m_DepthWaveOpsPSO.Finalize();

    m_ModelWaveOpsPSO = m_ModelPSO;
    m_ModelWaveOpsPSO.SetVertexShader( MY_SHADER_ARGS(g_pModelViewerVS_SM6) );
    m_ModelWaveOpsPSO.SetPixelShader( MY_SHADER_ARGS(g_pModelViewerPS_SM6) );
    m_ModelWaveOpsPSO.Finalize();
#endif

    m_CutoutModelPSO = m_ModelPSO;
    m_CutoutModelPSO.SetRasterizerState(RasterizerTwoSided);
    m_CutoutModelPSO.Finalize();

    // A debug shader for counting lights in a tile
    m_WaveTileCountPSO = m_ModelPSO;
    // m_WaveTileCountPSO.SetPixelShader(MY_SHADER_ARGS(g_pWaveTileCountPS) );
    m_WaveTileCountPSO.Finalize();

    Lighting::InitializeResources();

    m_ExtraTextures[0] = g_SSAOFullScreen.GetSRV();
    // m_ExtraTextures[1] = g_ShadowBuffer.GetSRV();

    TextureManager::Initialize(L"Textures/");
    ASSERT(m_Model.Load("Models/sponza.h3d"), "Failed to load model");
    ASSERT(m_Model.m_Header.meshCount > 0, "Model contains no meshes");

    // The caller of this function can override which materials are considered cutouts
    m_pMaterialIsCutout.resize(m_Model.m_Header.materialCount);
    for (uint32_t i = 0; i < m_Model.m_Header.materialCount; ++i)
    {
        const Model::Material& mat = m_Model.m_pMaterial[i];
        if (std::string(mat.texDiffusePath).find("thorn") != std::string::npos ||
            std::string(mat.texDiffusePath).find("plant") != std::string::npos ||
            std::string(mat.texDiffusePath).find("chain") != std::string::npos)
        {
            m_pMaterialIsCutout[i] = true;
        }
        else
        {
            m_pMaterialIsCutout[i] = false;
        }
    }

    // CreateParticleEffects();

    float modelRadius = Length(m_Model.m_Header.boundingBox.max - m_Model.m_Header.boundingBox.min) * .5f;
    const Vector3 eye = (m_Model.m_Header.boundingBox.min + m_Model.m_Header.boundingBox.max) * .5f + Vector3(modelRadius * .5f, 0.0f, 0.0f);
    m_Camera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
    m_Camera.SetZRange( 1.0f, 10000.0f );
    m_CameraController.reset(new CameraController(m_Camera, Vector3(kYUnitVector)));

    MotionBlur::Enable = true;
    TemporalEffects::EnableTAA = true;
    FXAA::Enable = true;
    // PostEffects::EnableHDR = true;
    // PostEffects::EnableAdaptation = true;
    SSAO::Enable = true;

    Lighting::CreateRandomLights(m_Model.GetBoundingBox().min, m_Model.GetBoundingBox().max);

    m_ExtraTextures[2] = Lighting::m_LightBuffer.GetSRV();
    m_ExtraTextures[3] = Lighting::m_LightShadowArray.GetSRV();
    m_ExtraTextures[4] = Lighting::m_LightGrid.GetSRV();
    m_ExtraTextures[5] = Lighting::m_LightGridBitMask.GetSRV();
}

void ModelViewer::Cleanup( void )
{
    m_DepthPSO.Destroy();
    m_CutoutDepthPSO.Destroy();
    m_ModelPSO.Destroy();
#ifdef _WAVE_OP
    m_DepthWaveOpsPSO.Destroy();
    m_ModelWaveOpsPSO.Destroy();
#endif
    m_CutoutModelPSO.Destroy();
    m_ShadowPSO.Destroy();
    m_CutoutShadowPSO.Destroy();
    m_WaveTileCountPSO.Destroy();

	m_Model.Clear();
    Lighting::Shutdown();
}

namespace Graphics
{
    extern EnumVar DebugZoom;
}

void ModelViewer::Update( float deltaT )
{
    ScopedTimer _prof(L"Update State");

    if (GameInput::IsFirstPressed(GameInput::kLShoulder))
        DebugZoom.Decrement();
    else if (GameInput::IsFirstPressed(GameInput::kRShoulder))
        DebugZoom.Increment();

    m_CameraController->Update(deltaT);
    m_ViewProjMatrix = m_Camera.GetViewProjMatrix();

    float costheta = cosf(m_SunOrientation);
    float sintheta = sinf(m_SunOrientation);
    float cosphi = cosf(m_SunInclination * 3.14159f * 0.5f);
    float sinphi = sinf(m_SunInclination * 3.14159f * 0.5f);
    m_SunDirection = Normalize(Vector3( costheta * cosphi, sinphi, sintheta * cosphi ));

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
}

void ModelViewer::RenderObjects( GraphicsContext& gfxContext, const Matrix4& ViewProjMat, eObjectFilter Filter )
{
    struct VSConstants
    {
        Matrix4 modelToProjection;
        Matrix4 modelToShadow;
        XMFLOAT3 viewerPos;
    } vsConstants;
    vsConstants.modelToProjection = ViewProjMat;
    // vsConstants.modelToShadow = m_SunShadow.GetShadowMatrix();
    XMStoreFloat3(&vsConstants.viewerPos, m_Camera.GetPosition());

    gfxContext.SetDynamicConstantBufferView(0, sizeof(vsConstants), &vsConstants, { kBindVertex } );

    uint32_t materialIdx = 0xFFFFFFFFul;

    uint32_t VertexStride = m_Model.m_VertexStride;

    for (uint32_t meshIndex = 0; meshIndex < m_Model.m_Header.meshCount; meshIndex++)
    {
        const Model::Mesh& mesh = m_Model.m_pMesh[meshIndex];

        uint32_t indexCount = mesh.indexCount;
        uint32_t startIndex = mesh.indexDataByteOffset / sizeof(uint16_t);
        uint32_t baseVertex = mesh.vertexDataByteOffset / VertexStride;

        if (mesh.materialIndex != materialIdx)
        {
            if ( m_pMaterialIsCutout[mesh.materialIndex] && !(Filter & kCutout) ||
                !m_pMaterialIsCutout[mesh.materialIndex] && !(Filter & kOpaque) )
                continue;

            materialIdx = mesh.materialIndex;
            gfxContext.SetDynamicDescriptors(0, 6, m_Model.GetSRVs(materialIdx), { kBindPixel } );
        }

        gfxContext.SetConstants(1, baseVertex, materialIdx, { kBindVertex });

        gfxContext.DrawIndexed(indexCount, startIndex, baseVertex);
    }
}

void ModelViewer::RenderScene( void )
{
    static bool s_ShowLightCounts = false;
    if (ShowWaveTileCounts != s_ShowLightCounts)
    {
        static bool EnableHDR;
        if (ShowWaveTileCounts)
        {
            // EnableHDR = PostEffects::EnableHDR;
            // PostEffects::EnableHDR = false;
        }
        else
        {
            // PostEffects::EnableHDR = EnableHDR;
        }
        s_ShowLightCounts = ShowWaveTileCounts;
    }

    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");

    __declspec(align(16)) struct
    {
        Vector3 sunDirection;
        Vector3 sunLight;
        Vector3 ambientLight;
        float ShadowTexelSize[4];

        float InvTileDim[4];
        uint32_t TileCount[4];
        uint32_t FirstLightIndex[4];
    } psConstants;

    psConstants.sunDirection = m_SunDirection;
    psConstants.sunLight = Vector3(1.0f, 1.0f, 1.0f) * m_SunLightIntensity;
    psConstants.ambientLight = Vector3(1.0f, 1.0f, 1.0f) * m_AmbientIntensity;
    // psConstants.ShadowTexelSize[0] = 1.0f / g_ShadowBuffer.GetWidth();
    psConstants.InvTileDim[0] = 1.0f / Lighting::LightGridDim;
    psConstants.InvTileDim[1] = 1.0f / Lighting::LightGridDim;
    psConstants.TileCount[0] = Math::DivideByMultiple(g_SceneColorBuffer.GetWidth(), Lighting::LightGridDim);
    psConstants.TileCount[1] = Math::DivideByMultiple(g_SceneColorBuffer.GetHeight(), Lighting::LightGridDim);
    psConstants.FirstLightIndex[0] = Lighting::m_FirstConeLight;
    psConstants.FirstLightIndex[1] = Lighting::m_FirstConeShadowedLight;

    // Set the default state for command lists
    auto pfnSetupGraphicsState = [&](void)
    {
        gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfxContext.SetIndexBuffer(m_Model.m_IndexBuffer.IndexBufferView());
        gfxContext.SetVertexBuffer(0, m_Model.m_VertexBuffer.VertexBufferView());
    };

    pfnSetupGraphicsState();

    if (!SSAO::DebugDraw)
    {
        ScopedTimer _prof(L"Main Render", gfxContext);

        gfxContext.ClearColor(g_SceneColorBuffer);
        gfxContext.ClearDepth(g_SceneDepthBuffer);

        pfnSetupGraphicsState();

        {
            ScopedTimer _prof(L"Render Color", gfxContext);

            gfxContext.SetDynamicDescriptors(0, _countof(m_ExtraTextures), m_ExtraTextures, { kBindPixel } );
            gfxContext.SetDynamicConstantBufferView(0, sizeof(psConstants), &psConstants, { kBindPixel } );
#ifdef _WAVE_OP
            gfxContext.SetPipelineState(EnableWaveOps ? m_ModelWaveOpsPSO : m_ModelPSO );
#else
            gfxContext.SetPipelineState(ShowWaveTileCounts ? m_WaveTileCountPSO : m_ModelPSO);
#endif
            D3D11_SAMPLER_HANDLE Sampler[] = { m_DefaultSampler, SamplerShadow };
            gfxContext.SetDynamicSamplers( 0, 2, Sampler, { kBindPixel } );
            gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());
            gfxContext.SetViewportAndScissor(m_MainViewport, m_MainScissor);

            RenderObjects( gfxContext, m_ViewProjMatrix, kOpaque );

            if (!ShowWaveTileCounts)
            {
                gfxContext.SetPipelineState(m_CutoutModelPSO);
                RenderObjects( gfxContext, m_ViewProjMatrix, kCutout );
            }
        }
    }

	gfxContext.Finish();
}