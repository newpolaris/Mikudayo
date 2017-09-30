#include "stdafx.h"

#include <random>
#pragma warning(push)
#pragma warning(disable: 4201)
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#pragma warning(pop)
#include "DeferredLighting.h"
#include "BufferManager.h"
#include "CommandContext.h"
#include "PipelineState.h"
#include "Camera.h"
#include "GraphicsCore.h"
#include "SceneNode.h"
#include "VectorMath.h"
#include "Color.h"
#include "OpaquePass.h"
#include "TransparentPass.h"
#include "PrimitiveUtility.h"
#include "DebugHelper.h"

#include "CompiledShaders/PmxColorVS.h"
#include "CompiledShaders/PmxColorPS.h"
#include "CompiledShaders/ScreenQuadVS.h"
#include "CompiledShaders/DeferredGBufferPS.h"
#include "CompiledShaders/DeferredLightingVS.h"
#include "CompiledShaders/DeferredLightingPS.h"
#include "CompiledShaders/DeferredLightingDebugPS.h"

using namespace Math;
using namespace Graphics;

namespace Lighting
{
    enum class LightType : uint32_t
    {
        Point = 0,
        Spot = 1,
        Directional = 2
    };
    struct LightData
    {
        Vector4 PositionWS;
        Vector4 PositionVS;
        Vector3 DirectionWS;
        Vector3 DirectionVS;
        Color Color;
        float Range;
        LightType Type;
        float SpotlightAngle;
    };
    LightData m_LightData[MaxLights];

    StructuredBuffer m_LightBuffer;
    ColorBuffer m_DiffuseTexture;
    ColorBuffer m_SpecularTexture;
    ColorBuffer m_NormalTexture;
    ColorBuffer m_PositionTexture;

    GraphicsPSO m_GBufferPSO;
    GraphicsPSO m_LightingPSO;
    GraphicsPSO m_TransparentPSO;
    GraphicsPSO m_Lighting1PSO;
    GraphicsPSO m_Lighting2PSO;
    GraphicsPSO m_LightDebugPSO;
    GraphicsPSO m_TextureDebugPSO;
    OpaquePass m_OaquePass;
    TransparentPass m_TransparentPass;

    void LightingPass( GraphicsContext& gfxContext, std::shared_ptr<SceneNode>& scene );
    void RenderSubPass( GraphicsContext& gfxContext );
}

inline Vector3 Convert(const glm::vec3& v )
{
    return Vector3( v.x, v.y, v.z );
}

inline glm::vec3 Convert(const Vector3& v )
{
    return glm::vec3( v.GetX(), v.GetY(), v.GetZ() );
}

void Lighting::CreateRandomLights( const Vector3 minBound, const Vector3 maxBound )
{
    Vector3 posScale = maxBound - minBound;
    Vector3 posBias = minBound;

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist( 0, 1 );
    std::normal_distribution<float> gaussian( -1, 1 );
    auto randFloat = [&]() {
        return dist( mt );
    };
    auto randNormalFloat = [&]() {
        return gaussian( mt );
    };
    auto randVecUniform = [&]() {
        return Vector3( randFloat(), randFloat(), randFloat() );
    };
    auto randVecNormal = [&]() {
        return Normalize(Vector3( randNormalFloat(), randNormalFloat(), randNormalFloat() ));
    };

    for (uint32_t n = 0; n < MaxLights; n++)
    {
        auto& light = m_LightData[n];

        light.PositionWS = Vector4(randVecUniform() * posScale + posBias, 1.f);
        light.DirectionWS = randVecNormal();

        Vector3 color = randVecUniform();
        float colorScale = randFloat() * 0.3f + 0.3f;
        color = color * colorScale;
        light.Color = Color(color.GetX(), color.GetY(), color.GetZ());

        light.Range = randFloat() * 100.f;
        light.SpotlightAngle = randFloat() * (60.f - 1.f) + 1.f;
        light.Type = LightType(randFloat() > 0.5);
    }

    m_LightData[0].Color = Color(1.f, 0.f, 0.f);
    m_LightData[0].Range = 30;
    m_LightData[0].Type = LightType(0);
    m_LightData[0].PositionWS = Vector4(0, 20, -20, 1);
    // m_LightData[0].PositionWS = Vector4(0, 20, -30, 1);

    m_LightData[1].Color = Color(0.f, 1.f, 0.f);
    m_LightData[1].Range = 50;
    m_LightData[1].Type = LightType(0);
    m_LightData[1].PositionWS = Vector4(0, 50, 10, 1);

    m_LightData[2].Color = Color(0.f, 0.f, 1.f);
    m_LightData[2].Range = 50;
    m_LightData[2].Type = LightType(1);
    m_LightData[2].PositionWS = Vector4(25, 25, 10, 1);

#if 0
    m_LightData[3].Color = Color(0.5f, 0.5f, 0.f);
    m_LightData[3].Range = 150;
    m_LightData[3].Type = 0;
    m_LightData[3].PositionWS = Vector4(25, 25, 25, 1);

    m_LightData[4].Color = Color(1.0f, 0.0f, 0.f);
    m_LightData[4].Range = 80;
    m_LightData[4].Type = 0;
    m_LightData[4].PositionWS = Vector4(-100, 30, 15, 1);
#endif
}

void Lighting::Initialize( void )
{
    const uint32_t width = g_NativeWidth, height = g_NativeHeight;

    m_DiffuseTexture.Create(L"Diffuse Buffer", width, height, 1, DXGI_FORMAT_R8G8B8A8_UNORM );
    m_SpecularTexture.Create(L"Specular Buffer", width, height, 1, DXGI_FORMAT_R8G8B8A8_UNORM );
    m_NormalTexture.Create( L"Normal Buffer", width, height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT );
    m_PositionTexture.Create( L"Position Buffer", width, height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT );

	InputDesc PmxLayout[] = {
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_ID", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "EDGE_FLAT", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

    m_GBufferPSO.SetInputLayout( _countof( PmxLayout ), PmxLayout );
    m_GBufferPSO.SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
    m_GBufferPSO.SetPixelShader( MY_SHADER_ARGS( g_pDeferredGBufferPS ) );
    m_GBufferPSO.SetDepthStencilState( DepthStateReadWrite );
    m_GBufferPSO.SetRasterizerState( RasterizerDefault );
    m_GBufferPSO.Finalize();

    m_LightingPSO.SetVertexShader( MY_SHADER_ARGS( g_pScreenQuadVS ) );
    m_LightingPSO.SetPixelShader( MY_SHADER_ARGS( g_pDeferredLightingPS ) );
    m_LightingPSO.SetBlendState( BlendAdditive );
    m_LightingPSO.SetDepthStencilState( DepthStateDisabled );
    m_LightingPSO.Finalize();

    m_TransparentPSO.SetInputLayout( _countof( PmxLayout ), PmxLayout );
    m_TransparentPSO.SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
    m_TransparentPSO.SetPixelShader( MY_SHADER_ARGS( g_pPmxColorPS ) );;
    m_TransparentPSO.SetBlendState( BlendTraditional );
    m_TransparentPSO.SetRasterizerState( RasterizerDefault );
    m_TransparentPSO.Finalize();

    m_LightDebugPSO.SetInputLayout( _countof( PrimitiveUtility::Desc ), PrimitiveUtility::Desc );
    m_LightDebugPSO.SetVertexShader( MY_SHADER_ARGS( g_pDeferredLightingVS ) );
    m_LightDebugPSO.SetPixelShader( MY_SHADER_ARGS( g_pDeferredLightingDebugPS ) );
    m_LightDebugPSO.SetRasterizerState( RasterizerWireframe );
    m_LightDebugPSO.Finalize();

    // Disable writing to the depth buffer.
    // Pass depth test if the light volume is behind scene geometry.
    D3D11_DEPTH_STENCIL_DESC depth1 = DepthStateReadOnlyReversed;
    depth1.StencilEnable = TRUE;
    depth1.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR_SAT;

    // Pipeline for deferred lighting (stage 1 to determine lit pixels)
    m_Lighting1PSO.SetInputLayout( _countof( PrimitiveUtility::Desc ), PrimitiveUtility::Desc );
    m_Lighting1PSO.SetVertexShader( MY_SHADER_ARGS( g_pDeferredLightingVS ) );
    m_Lighting1PSO.SetRasterizerState( RasterizerDefault );
    m_Lighting1PSO.SetDepthStencilState( depth1 );
    m_Lighting1PSO.SetStencilRef( 1 );
    m_Lighting1PSO.Finalize();

    D3D11_RASTERIZER_DESC raster2 = RasterizerDefault;
    raster2.CullMode = D3D11_CULL_FRONT;
    raster2.DepthClipEnable = FALSE;

    // Setup depth mode
    // Disable depth writes
    D3D11_DEPTH_STENCIL_DESC depth2 = DepthStateReadOnly;
	depth2.DepthFunc = Math::g_ReverseZ ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_GREATER_EQUAL;
    depth2.StencilEnable = TRUE;
    // Render pixel if the depth function passes and the stencil was not un-marked in the previous pass.
    depth2.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

    // Pipeline for deferred lighting (stage 2 to render lit pixels)
    m_Lighting2PSO = m_Lighting1PSO;
    m_Lighting2PSO.SetPixelShader( MY_SHADER_ARGS( g_pDeferredLightingPS ) );
    m_Lighting2PSO.SetRasterizerState( raster2 );
    // Perform additive blending if a pixel passes the depth/stencil tests.
    m_Lighting2PSO.SetBlendState( BlendAdditive );
    m_Lighting2PSO.SetDepthStencilState( depth2 );
    m_Lighting2PSO.Finalize();

    // Pipeline for directional lights in deferred shader (only requires a single pass)
    /*
    {
        g_pDirectionalLightsPipeline = renderDevice.CreatePipelineState();
        g_pDirectionalLightsPipeline->SetShader( Shader::VertexShader, g_pVertexShader );
        g_pDirectionalLightsPipeline->SetShader( Shader::PixelShader, g_pDeferredLightingPixelShader );
        g_pDirectionalLightsPipeline->SetRenderTarget( renderWindow.GetRenderTarget() );
        g_pDirectionalLightsPipeline->GetBlendState().SetBlendMode( additiveBlending );

        // Setup depth mode
        DepthStencilState::DepthMode depthMode( true, DepthStencilState::DepthWrite::Disable ); // Disable depth writes.
        // The full-screen quad that will be used to light pixels will be placed at the far clipping plane.
        // Only light pixels that are "in front" of the full screen quad (exclude sky box pixels)
        depthMode.DepthFunction = DepthStencilState::CompareFunction::Greater;
        g_pDirectionalLightsPipeline->GetDepthStencilState().SetDepthMode( depthMode );
    }
    */
}

using namespace Lighting;
Matrix4 GetLightTransfrom(const LightData& Data)
{
    if ( Data.Type == LightType::Directional )
    {
        // return perObjectData.ModelViewProjection = m_pAlignedProperties->m_OrthographicProjection;
    }
    else
    {
        // glm::mat4 rotation = glm::toMat4( glm::quat( glm::vec3( 0, 0, 1 ), glm::normalize( glm::vec3( m_pCurrentLight->m_DirectionWS ) ) ) );
        Quaternion quat = RotationBetweenVectors( Vector3( 0, 0, 1 ), Data.DirectionWS );
        OrthogonalTransform transform( quat, Vector3( Data.PositionWS ) );

        // Compute the scale depending on the light type.
        float scaleX, scaleY, scaleZ;
        // For point lights, we want to scale the geometry by the range of the light.
        scaleX = scaleY = scaleZ = Data.Range;
        if ( Data.Type == LightType::Spot )
        {
            // For spotlights, we want to scale the base of the cone by the spotlight angle.
            scaleX = scaleY = glm::tan( glm::radians( Data.SpotlightAngle ) ) * Data.Range;
        }
        AffineTransform scale = AffineTransform::MakeScale( Vector3(scaleX, scaleY, scaleZ) );
        return AffineTransform(transform) * scale;
    }
}

void Lighting::LightingPass( GraphicsContext& gfxContext, std::shared_ptr<SceneNode>& scene )
{
    ScopedTimer _prof( L"Lighting Pass", gfxContext );
    D3D11_SRV_HANDLE srvs[] = {
        m_DiffuseTexture.GetSRV(),
        m_SpecularTexture.GetSRV(),
        m_NormalTexture.GetSRV(),
        m_PositionTexture.GetSRV()
    };

    gfxContext.SetDynamicDescriptors( 0, _countof( srvs ), srvs, { kBindPixel } );
    gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
    gfxContext.SetPipelineState( m_LightingPSO );

    for (uint32_t i = 0; i < MaxLights; i++)
    {
        __declspec(align(16)) uint32_t idx = i;
        gfxContext.SetDynamicConstantBufferView( 4, sizeof(uint32_t), &idx, { kBindPixel } );
        // Clear the stencil buffer for the next light
        gfxContext.ClearStencil( g_SceneDepthBuffer, 1 );

        LightData& light = m_LightData[i];
        __declspec(align(16)) Matrix4 model = GetLightTransfrom( light );
        gfxContext.SetDynamicConstantBufferView( 2, sizeof(Matrix4), &model, { kBindVertex } );
        switch (light.Type)
        {
        case LightType::Point:
        #if 1
            gfxContext.SetPipelineState( m_LightDebugPSO );
            gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
            PrimitiveUtility::Render( gfxContext, PrimitiveUtility::kSphereMesh );
        #endif

            gfxContext.SetPipelineState( m_Lighting1PSO );
            gfxContext.SetRenderTargets( 0, nullptr, g_SceneDepthBuffer.GetDSV() );
            PrimitiveUtility::Render( gfxContext, PrimitiveUtility::kSphereMesh );

            gfxContext.SetPipelineState( m_Lighting2PSO );
            gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
            PrimitiveUtility::Render( gfxContext, PrimitiveUtility::kSphereMesh );
            break;
        case LightType::Spot:
            // RenderSubPass( e, m_pSpotLightScene, m_LightPipeline0 );
            // RenderSubPass( e, m_pSpotLightScene, m_LightPipeline1 );
            break;
        case LightType::Directional:
            // RenderSubPass( e, m_pDirectionalLightScene, m_DirectionalLightPipeline );
            break;
        }
    }
}

void Lighting::RenderSubPass( GraphicsContext& gfxContext )
{
    PrimitiveUtility::Render( gfxContext, PrimitiveUtility::kSphereMesh );
}

void Lighting::Render( GraphicsContext& gfxContext, std::shared_ptr<SceneNode>& scene )
{
    gfxContext.ClearColor( m_DiffuseTexture );
    gfxContext.ClearColor( m_SpecularTexture );
    gfxContext.ClearColor( m_NormalTexture );
    gfxContext.ClearColor( m_PositionTexture );
    {
        ScopedTimer _prof( L"Geometry Pass", gfxContext );
        D3D11_RTV_HANDLE rtvs[] = {
            g_SceneColorBuffer.GetRTV(), // Amibent directly write into the scene buffer
            m_DiffuseTexture.GetRTV(),
            m_SpecularTexture.GetRTV(),
            m_NormalTexture.GetRTV(),
            m_PositionTexture.GetRTV()
        };
        gfxContext.SetRenderTargets( _countof( rtvs ), rtvs, g_SceneDepthBuffer.GetDSV() );
        gfxContext.SetPipelineState( m_GBufferPSO );
        scene->Render( gfxContext, m_OaquePass );
        D3D11_RTV_HANDLE nullrtvs[_countof( rtvs )] = { nullptr, };
        gfxContext.SetRenderTargets( _countof( rtvs ), nullrtvs, nullptr );
    }
    LightingPass( gfxContext, scene );
    gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
    {
        ScopedTimer _prof( L"Forward Pass", gfxContext );
        gfxContext.SetPipelineState( m_TransparentPSO );
        scene->Render( gfxContext, m_TransparentPass );
    }
}

void Lighting::Shutdown( void )
{
    m_LightBuffer.Destroy();

    m_DiffuseTexture.Destroy();
    m_SpecularTexture.Destroy();
    m_NormalTexture.Destroy();
    m_PositionTexture.Destroy();
}

void Lighting::UpdateLights( const Camera& C )
{
    const Matrix4& View = C.GetViewMatrix();
    for (uint32_t n = 0; n < MaxLights; n++)
    {
        auto& light = m_LightData[n];
        light.PositionVS = View * light.PositionWS;
        light.DirectionVS = View.Get3x3() * light.DirectionWS;
    }
    m_LightBuffer.Create(L"m_LightBuffer", MaxLights, sizeof(LightData), m_LightData);
}
