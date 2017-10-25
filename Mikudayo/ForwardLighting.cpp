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
#include "Material.h"
#include "ModelManager.h"
#include "PmxModel.h"
#include "SceneNode.h"

#include "CompiledShaders/PmxColorVS.h"
#include "CompiledShaders/PmxColorPS.h"

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

    BasicPass m_BasicPass;
    OutlinePass m_OutlinePass;
    TwoSidedPass m_TwoSidedPass;

    std::shared_ptr<Scene> m_MirrorScene;
    GraphicsPSO m_MirrorPSO;
    GraphicsPSO m_MirroredPSO;
};

void Forward::Initialize( void )
{
    m_MirrorScene = std::make_shared<Scene>();
    SceneNodePtr mirror = ModelManager::Load( L"Model/Villa Fortuna Stage/MirrorWF/MirrorWF.pmx" );
    OrthogonalTransform rotation( Quaternion( -3.14/2, 0, 0 ) );
    mirror->SetTransform( rotation );
    m_MirrorScene->AddChild( mirror );

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

        RenderPass pass;
        {
            ScopedTimer _prof( L"Stencil Pass", gfxContext );
            gfxContext.ClearStencil( g_SceneDepthBuffer, 0 );
            gfxContext.SetPipelineState( m_MirrorPSO );
            m_MirrorScene->Render( pass, args );
        }
        {
            ScopedTimer _prof( L"Mirror Pass", gfxContext );
            args.m_ModelMatrix = Matrix4::MakeScale( Vector3( 1, -1, 1 ) );
            gfxContext.SetPipelineState( m_MirroredPSO );
            scene->Render( pass, args );
            args.m_ModelMatrix = Matrix4( kIdentity );
            gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
        }
    }
    {
        ScopedTimer _prof( L"Outline Pass", gfxContext );
        scene->Render( m_OutlinePass, args );
    }
}

void Forward::Shutdown( void )
{
    m_MirrorScene.reset();
}