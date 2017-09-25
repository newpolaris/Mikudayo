#include "stdafx.h"

#include "VectorMath.h"
#include "Color.h"

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

#include "CompiledShaders/PmxColorVS.h"
// #include "CompiledShaders/ScreenQuadVS.h"

using namespace Math;

namespace Lighting
{
    LightData m_LightData[MaxLights];

    StructuredBuffer m_LightBuffer;
    ColorBuffer m_LightAccTexture;
    ColorBuffer m_DiffuseTexture;
    ColorBuffer m_SpecularTexture;
    ColorBuffer m_NormalTexture;

    GraphicsPSO m_GeometryPSO;
    GraphicsPSO m_LightingPSO;
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

        light.Range = randFloat() * 80.f + 20.f;
        light.SpotlightAngle = randFloat() * (60.f - 1.f) + 1.f;
        light.Type = (randFloat() > 0.5);
    }
}

void Lighting::Initialize( void )
{
    const uint32_t width = Graphics::g_NativeWidth, height = Graphics::g_NativeHeight;

    m_LightAccTexture.Create(L"Diffuse Buffer", width, height, 1, DXGI_FORMAT_R32_UINT);
    m_DiffuseTexture.Create(L"Diffuse Buffer", width, height, 1, DXGI_FORMAT_R32_UINT);
    m_SpecularTexture.Create(L"Diffuse Buffer", width, height, 1, DXGI_FORMAT_R32_UINT);
    m_NormalTexture.Create(L"Diffuse Buffer", width, height, 1, DXGI_FORMAT_R32_UINT);

	InputDesc Layout[] = {
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_ID", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "EDGE_FLAT", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

    m_GeometryPSO.SetInputLayout(_countof(Layout), Layout);
    m_GeometryPSO.SetVertexShader(MY_SHADER_ARGS(g_pPmxColorVS));
    // m_GeometryPSO.SetPixelShader( MY_SHADER_ARGS( g_pBulletPrimitivePS ) );
    m_GeometryPSO.SetDepthStencilState(Graphics::DepthStateReadWrite);
    m_GeometryPSO.SetRasterizerState(Graphics::RasterizerDefault);
    m_GeometryPSO.Finalize();

    // m_LightingPSO.SetVertexShader(MY_SHADER_ARGS(g_pScreenQuadVS));
}

void Lighting::Render(GraphicsContext& gfxContext)
{
    ScopedTimer _prof(L"Geometry Pass", gfxContext);
    D3D11_RTV_HANDLE rtvs[] = { 
        m_LightAccTexture.GetRTV(), 
        m_DiffuseTexture.GetRTV(),
        m_SpecularTexture.GetRTV(),
        m_NormalTexture.GetRTV() 
    };
    gfxContext.SetRenderTargets( _countof(rtvs), rtvs, Graphics::g_SceneDepthBuffer.GetDSV() );
    gfxContext.SetPipelineState( m_GeometryPSO );

    // gfxContext.SetRenderTargets( _countof(rtvs), rtvs, Graphics::g_SceneDepthBuffer.GetDSV());
    // gfxContext.SetPipelineState(m_GeometryPSO);
}

void Lighting::Shutdown( void )
{
    m_LightBuffer.Destroy();
}

void Lighting::UpdateLights( const Camera& C )
{
    const Matrix4& View = C.GetViewMatrix();
    for (uint32_t n = 0; n < MaxLights; n++)
    {
        auto& light = Lighting::m_LightData[n];
        light.PositionVS = View * light.PositionWS;
        light.DirectionVS = View.Get3x3() * light.DirectionWS;
    }
    m_LightData[0].Color = Color(1.f, 0.f, 0.f);
    m_LightData[0].Range = 50;
    m_LightData[0].Type = 0;
    m_LightData[0].PositionWS = Vector4(0, 0, 0, 1);

    m_LightData[1].Color = Color(0.f, 1.f, 0.f);
    m_LightData[1].Range = 100;
    m_LightData[1].Type = 0;
    m_LightData[1].PositionWS = Vector4(0, 50, 0, 1);

    m_LightData[2].Color = Color(1.f, 0.f, 0.f);
    m_LightData[2].Range = 50;
    m_LightData[2].Type = 0;
    m_LightData[2].PositionWS = Vector4(25, 25, 0, 1);

    m_LightData[3].Color = Color(0.5f, 0.5f, 0.f);
    m_LightData[3].Range = 150;
    m_LightData[3].Type = 0;
    m_LightData[3].PositionWS = Vector4(25, 25, 25, 1);

    m_LightData[4].Color = Color(1.0f, 0.0f, 0.f);
    m_LightData[4].Range = 80;
    m_LightData[4].Type = 0;
    m_LightData[4].PositionWS = Vector4(-100, 30, 3, 1);

    m_LightBuffer.Create(L"m_LightBuffer", MaxLights, sizeof(LightData), m_LightData);
}
