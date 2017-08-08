#include "BulletDebugDraw.h"

#include <string>
#include <vector>
#include "GameCore.h"
#include "GraphicsCore.h"
#include "TextRenderer.h"
#include "CommandContext.h"
#include "PipelineState.h"
#include "GpuBuffer.h"
#include "InputLayout.h"

#include "CompiledShaders/BulletLineVS.h"
#include "CompiledShaders/BulletLinePS.h"

using namespace std;
using namespace Math;

struct Vertex
{
    Vertex() {}
    Vertex( const btVector3& p, const btVector3& c );

    XMFLOAT3 position;
    XMFLOAT3 color;
};

Vertex::Vertex( const btVector3& p, const btVector3& c )
{
    position.x = p[0], position.y = p[1], position.z = p[2];
    color.x = c[0], color.y = c[1], color.z = c[2];
}

namespace BulletDebug
{
    void Initialize();
    void Shutdown();

    using Text3D = std::pair<string, Vector3>;
    vector<string> Warning;
    vector<Text3D> Text;
    vector<Vertex> Lines;
    GraphicsPSO BulletDebugLinePSO;
}

using namespace BulletDebug;

void BulletDebug::Initialize()
{
    InputDesc Desc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    BulletDebugLinePSO.SetPrimitiveTopologyType( D3D_PRIMITIVE_TOPOLOGY_LINELIST );
    BulletDebugLinePSO.SetVertexShader( MY_SHADER_ARGS(g_pBulletLineVS) );
    BulletDebugLinePSO.SetPixelShader( MY_SHADER_ARGS(g_pBulletLinePS) );
    BulletDebugLinePSO.SetInputLayout( _countof(Desc), Desc );
    BulletDebugLinePSO.SetDepthStencilState( Graphics::DepthStateDisabled );
    BulletDebugLinePSO.Finalize();
}

void BulletDebug::Shutdown()
{
    BulletDebugLinePSO.Destroy();
}

void BulletDebugDraw::drawLine( const btVector3& from, const btVector3& to, const btVector3& color )
{
    Lines.emplace_back( Vertex(from, color) );
    Lines.emplace_back( Vertex(to, color) );
}

void BulletDebugDraw::reportErrorWarning( const char* warningString )
{
    Warning.emplace_back(string(warningString));
}

void BulletDebugDraw::draw3dText( const btVector3& location, const char* textString )
{
    Text.emplace_back( Text3D{ string(textString), Vector3(location.get128()) } );
}

void BulletDebugDraw::flush( GraphicsContext& UiContext, const Matrix4& WorldToClip )
{
    ScopedTimer _prof( L"Bullet Debug Draw", UiContext );

    if (Lines.size() > 0)
    {
        struct
        {
            Matrix4 WorldToClip;
        } vsConstant;
        vsConstant.WorldToClip = WorldToClip;

        UiContext.SetDynamicConstantBufferView( 0, sizeof(vsConstant), &vsConstant, { kBindVertex } );
        UiContext.SetPipelineState( BulletDebugLinePSO );
        UiContext.SetDynamicVB( 0, Lines.size(), sizeof(Vertex), Lines.data() );
        UiContext.Draw( static_cast<UINT>(Lines.size()), 0 );
    }
    Lines.clear();

    // Print reportErrorWarning
    const XMFLOAT2 WarningPos( 10.f, 100.f );
	TextContext Context(UiContext);
	Context.Begin();
    Context.ResetCursor( WarningPos.x, WarningPos.y );
    Context.SetColor(Color( 0.5f, 0.0f, 0.f ));
    for (auto& text : Warning)
    {
        Context.DrawString( text );
        Context.NewLine();
    }
    Warning.clear();

    // Print draw3dText
    const Vector3 DisplayScale(Graphics::g_DisplayWidth, Graphics::g_DisplayHeight, 1.0f);
    AffineTransform S( Matrix3::MakeScale( 0.5f, 0.5f, 1.0f ), Vector3( 0.5f, 0.5f, 0.0f ) );
    AffineTransform T = AffineTransform::MakeScale( DisplayScale ) * S;

    for (auto& text : Text)
    {
        Vector3 Coord = T * text.second;
        Context.ResetCursor( Coord.GetX(), Coord.GetY() );
        Context.DrawString( text.first );
    }
    Text.clear();
}
