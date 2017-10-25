#include "stdafx.h"

#include <random>
#include "ForwardLighting.h"
#include "BufferManager.h"
#include "CommandContext.h"
#include "PipelineState.h"
#include "Camera.h"
#include "GraphicsCore.h"
#include "Scene.h"
#include "VectorMath.h"
#include "Color.h"
#include "OpaquePass.h"
#include "TransparentPass.h"
#include "OutlinePass.h"
#include "PrimitiveUtility.h"
#include "DebugHelper.h"
#include "RenderArgs.h"
#include "Scene.h"
#include "Material.h"

#include "ModelManager.h"
#include "PmxModel.h"
#include "SceneNode.h"

#include "CompiledShaders/PmxColorVS.h"
#include "CompiledShaders/PmxColorPS.h"
#include "CompiledShaders/PmxMirrorPS.h"

using namespace Math;
using namespace Graphics;

namespace Forward
{
    class BasicPass : public RenderPass
    {
    public:

        BasicPass() : RenderPass( kRenderQueueTransparent ) {}
        virtual bool Enable( IMaterial& material ) {
            return !material.IsTwoSided();
        }
    };

    class TwoSidedPass : public RenderPass
    {
    public:

        TwoSidedPass() : RenderPass( kRenderQueueTransparentTwoSided ) {}
        virtual bool Enable( IMaterial& material ) {
            return material.IsTwoSided();
        }
    };

    class MirrorPass : public RenderPass
    {
    public:

        MirrorPass(ScenePtr& scene) : 
            RenderPass( kRenderQueueEmpty ), m_Scene(scene) {}

        bool Enable( SceneNode& node ) override;
        bool Visit( SceneNode& node ) override;

        ScenePtr& m_Scene;
    };

    struct MirroredPass : public RenderPass {
        MirroredPass( SceneNode& node ) : m_Node( node ) {}
        bool Enable( SceneNode& node ) override
        {
            return &m_Node != &node;
        }
        SceneNode& m_Node;
    };

    BasicPass m_BasicPass;
    OutlinePass m_OutlinePass;
    TwoSidedPass m_TwoSidedPass;
    GraphicsPSO m_MirrorPSO;
    GraphicsPSO m_MirroredPSO;
    GraphicsPSO m_ReflectPSO;
};

bool Forward::MirrorPass::Enable( SceneNode& node )
{
    return kSceneMirror == node.GetType();
}

bool Forward::MirrorPass::Visit( SceneNode& node )
{
    if (!Enable( node ))
        return false;
    if (m_RenderArgs == nullptr)
        return false;
    GraphicsContext& context = m_RenderArgs->gfxContext;

    context.ClearDepth( g_ReflectDepthBuffer );
    context.ClearColor( g_ReflectEmissiveBuffer );
    context.ClearColor( g_ReflectColorBuffer );

    D3D11_RTV_HANDLE rtvs[] = {
        g_ReflectColorBuffer.GetRTV(),
        g_ReflectEmissiveBuffer.GetRTV(),
    };
    context.SetRenderTargets( _countof( rtvs ), rtvs, g_ReflectDepthBuffer.GetDSV() );

    // Stencil mask
    ScopedTimer _prof( L"Stencil Pass", context );
    context.ClearStencil( g_SceneDepthBuffer, 0 );
    context.SetPipelineState( m_MirrorPSO );
    Matrix4 model = node.GetTransform();
    context.SetDynamicConstantBufferView( 2, sizeof( model ), &model, { kBindVertex } );
    RenderPass pass;
    node.Render( context, pass );

    // Render with clip enabled shader
    MirroredPass mirrorPass( node );
    m_RenderArgs->m_ModelMatrix = Matrix4::MakeScale( Vector3( 1, -1, 1 ) );
    Vector4 reflector( 0, 1, 0, 0 );
    context.SetDynamicConstantBufferView( 11, sizeof( reflector ), &reflector, { kBindPixel } );
    context.SetPipelineState( m_MirroredPSO );
    m_Scene->Render( mirrorPass, *m_RenderArgs );
    m_RenderArgs->m_ModelMatrix = Matrix4( kIdentity );
    D3D11_RTV_HANDLE rtvs2[] = {
        g_SceneColorBuffer.GetRTV(),
        g_EmissiveColorBuffer.GetRTV(),
    };
    context.SetRenderTargets( _countof( rtvs2 ), rtvs2, g_SceneDepthBuffer.GetDSV() );

    // Render mirror with alpha-blend

    context.SetDynamicConstantBufferView( 2, sizeof( model ), &model, { kBindVertex } );
    context.SetDynamicDescriptor( 63, g_ReflectColorBuffer.GetSRV(), { kBindPixel } );
    context.SetDynamicDescriptor( 64, g_ReflectEmissiveBuffer.GetSRV(), { kBindPixel } );
    context.SetPipelineState( m_ReflectPSO );
    node.Render( context, *this );

    return true;
}

void Forward::Initialize( void )
{
    D3D11_DEPTH_STENCIL_DESC depth1 = DepthStateReadOnly;
    depth1.StencilEnable = TRUE;
    depth1.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;

    m_MirrorPSO.SetInputLayout( (UINT)Pmx::VertElem.size(), Pmx::VertElem.data() );
    m_MirrorPSO.SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
    m_MirrorPSO.SetRasterizerState( RasterizerDefault );
    m_MirrorPSO.SetDepthStencilState( depth1 );
    m_MirrorPSO.SetStencilRef( 1 );
    m_MirrorPSO.Finalize();

    D3D11_DEPTH_STENCIL_DESC depth2 = DepthStateReadWrite;
    depth2.StencilEnable = TRUE;
    depth2.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

    m_MirroredPSO.SetInputLayout( (UINT)Pmx::VertElem.size(), Pmx::VertElem.data() );
    m_MirroredPSO.SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
    m_MirroredPSO.SetPixelShader( MY_SHADER_ARGS( g_pPmxColorPS ) );
    m_MirroredPSO.SetRasterizerState( RasterizerDefaultCW );
    m_MirroredPSO.SetDepthStencilState( depth2 );
    m_MirroredPSO.SetBlendState( BlendTraditional );
    m_MirroredPSO.SetStencilRef( 1 );
    m_MirroredPSO.Finalize();

    m_ReflectPSO.SetInputLayout( (UINT)Pmx::VertElem.size(), Pmx::VertElem.data() );
    m_ReflectPSO.SetVertexShader( MY_SHADER_ARGS( g_pPmxColorVS ) );
    m_ReflectPSO.SetPixelShader( MY_SHADER_ARGS( g_pPmxMirrorPS ) );
    m_ReflectPSO.SetRasterizerState( RasterizerDefault );
    m_ReflectPSO.SetBlendState( BlendTraditional );
    m_ReflectPSO.SetDepthStencilState( DepthStateReadWrite );
    m_ReflectPSO.Finalize();
}

void Forward::Render( std::shared_ptr<Scene>& scene, RenderArgs& args )
{
    GraphicsContext& gfxContext = args.gfxContext;
    {
        ScopedTimer _prof( L"Forward Pass", gfxContext );
        gfxContext.ClearColor( g_EmissiveColorBuffer );
        D3D11_RTV_HANDLE rtvs[] = {
            g_SceneColorBuffer.GetRTV(),
            g_EmissiveColorBuffer.GetRTV(),
        };
        gfxContext.SetRenderTargets( _countof( rtvs ), rtvs, g_SceneDepthBuffer.GetDSV() );
        scene->Render( m_BasicPass, args );
        scene->Render( m_TwoSidedPass, args );
    }
    {
        ScopedTimer _prof( L"Outline Pass", gfxContext );
        scene->Render( m_OutlinePass, args );
    }

    MirrorPass mirror( scene );
    scene->Render( mirror, args );

    D3D11_RTV_HANDLE rtvs[] = {
        g_SceneColorBuffer.GetRTV(),
        g_EmissiveColorBuffer.GetRTV(),
    };
    gfxContext.SetRenderTargets( _countof( rtvs ), rtvs, g_SceneDepthBuffer.GetDSV() );
}

void Forward::Shutdown( void )
{
}