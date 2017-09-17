#include "stdafx.h"
#include "BulletDebugDraw.h"

#include "CompiledShaders/BulletLineVS.h"
#include "CompiledShaders/BulletLinePS.h"

using namespace Math;
namespace BulletDebug
{
    template <typename T, size_t S>
    class Array
    {
    public:
        Array() : m_Index( 0 )
        {
        }

        T* data( void ) { return m_Data.data(); }
        T& operator[]( size_t Pos ) { return m_Data[Pos]; }
        void resize( size_t Pos ) { m_Index = Pos; }
        void clear( void ) { m_Index = 0; }
        size_t size( void ) const { return m_Index; }

        template <typename... Args>
        void emplace_back( Args&&... Val )
        {
            m_Data[m_Index++] = std::move( T( std::forward<Args>( Val )... ) );
        }

    private:
        std::array<T, S> m_Data;
        size_t m_Index;
    };

    struct Vertex
    {
        Vertex() {}
        Vertex( const btVector3& p, const btVector3& c );

        XMFLOAT3 position;
        XMFLOAT3 color;
    };

#define FAST_LINE_DRAW 0
#if FAST_LINE_DRAW
    using LineStorage = Array<Vertex, 1 << 20>; // Slightly faster but have limit size
#else
    using LineStorage = std::vector<Vertex>;
#endif

	GraphicsPSO BulletDebugLinePSO;
}

void BulletDebug::Initialize()
{
    InputDesc BulletDebugInputDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    BulletDebugLinePSO.SetPrimitiveTopologyType( D3D_PRIMITIVE_TOPOLOGY_LINELIST );
    BulletDebugLinePSO.SetVertexShader( MY_SHADER_ARGS(g_pBulletLineVS) );
    BulletDebugLinePSO.SetPixelShader( MY_SHADER_ARGS(g_pBulletLinePS) );
    BulletDebugLinePSO.SetInputLayout( _countof(BulletDebugInputDesc), BulletDebugInputDesc );
    BulletDebugLinePSO.SetDepthStencilState( Graphics::DepthStateDisabled );
    BulletDebugLinePSO.Finalize();
}

void BulletDebug::Shutdown()
{
}

using namespace BulletDebug;

Vertex::Vertex( const btVector3& p, const btVector3& c )
{
    position.x = p[0], position.y = p[1], position.z = p[2];
    color.x = c[0], color.y = c[1], color.z = c[2];
}

using Text3D = std::pair<std::string, Vector3>;
struct DebugDraw::Context
{
    std::vector<std::string> Warning;
    std::vector<Text3D> Text;
    LineStorage Lines;
};

DebugDraw::DebugDraw() :
    m_Context(std::make_shared<Context>())
{
}

void DebugDraw::drawLine( const btVector3& from, const btVector3& to, const btVector3& color )
{
    m_Context->Lines.emplace_back(from, color);
    m_Context->Lines.emplace_back(to, color);
}

void DebugDraw::reportErrorWarning( const char* warningString )
{
    m_Context->Warning.emplace_back(warningString);
}

void DebugDraw::draw3dText( const btVector3& location, const char* textString )
{
    m_Context->Text.emplace_back( std::string(textString), Vector3(location.get128()) );
}

void DebugDraw::flush( GraphicsContext& UiContext, const Matrix4& WorldToClip )
{
    ScopedTimer _prof( L"Bullet Debug Draw", UiContext );

    auto& Lines = m_Context->Lines;
    if (Lines.size() > 0)
    {
        struct
        {
            Matrix4 WorldToClip;
        } vsConstant;
        vsConstant.WorldToClip = WorldToClip;

        UiContext.SetDynamicConstantBufferView( 0, sizeof(vsConstant), &vsConstant, { kBindVertex } );
        UiContext.SetPipelineState( BulletDebug::BulletDebugLinePSO );
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
    for (auto& text : m_Context->Warning)
    {
        Context.DrawString( text );
        Context.NewLine();
    }
    m_Context->Warning.clear();

    // Print draw3dText
    const Vector3 DisplayScale((float)Graphics::g_DisplayWidth, (float)Graphics::g_DisplayHeight, 1.0f);
    AffineTransform S( Matrix3::MakeScale( 0.5f, 0.5f, 1.0f ), Vector3( 0.5f, 0.5f, 0.0f ) );
    AffineTransform T = AffineTransform::MakeScale( DisplayScale ) * S;

    for (auto& text : m_Context->Text)
    {
        Vector3 Coord = T * text.second;
        Context.ResetCursor( Coord.GetX(), Coord.GetY() );
        Context.DrawString( text.first );
    }
    m_Context->Text.clear();
}
