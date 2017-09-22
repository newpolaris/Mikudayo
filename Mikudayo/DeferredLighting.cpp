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

using namespace Math;

namespace Lighting
{
    LightData m_LightData[MaxLights];

    StructuredBuffer m_LightBuffer;
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
        light.Type = (randFloat() > 0.5);
    }
}

void Lighting::Initialize( void )
{
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
    m_LightBuffer.Create(L"m_LightBuffer", MaxLights, sizeof(LightData), m_LightData);
}
