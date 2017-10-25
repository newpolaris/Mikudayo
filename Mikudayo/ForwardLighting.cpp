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

using namespace Math;
using namespace Graphics;

namespace Forward
{
    class DefaultPass : public RenderPass
    {
    public:

        DefaultPass( RenderQueue Queue = kRenderQueueOpaque ) : RenderPass( Queue ), m_BaseQueue( Queue ) {}
        uint32_t GetOffset( IMaterial& material ) {
            if (!material.IsTransparent())
                return !material.IsTwoSided() ? kRenderQueueOpaque : kRenderQueueOpaqueTwoSided;
            else
                return !material.IsTwoSided() ? kRenderQueueTransparent : kRenderQueueTransparentTwoSided;
        }
        virtual bool Enable( IMaterial& material ) {
            RenderQueue target = RenderQueue(m_BaseQueue + GetOffset( material ));
            SetRenderQueue( target );
            return true;
        }
        RenderQueue m_BaseQueue;
    };

    class MirrorPass : public RenderPass
    {
    public:

        MirrorPass(ScenePtr& scene) : 
            RenderPass( kRenderQueueTransparent ), m_Scene(scene) {}

        bool Enable( SceneNode& node ) override;
        bool Visit( SceneNode& node ) override;

        ScenePtr& m_Scene;
    };

    struct ReflectPass : public DefaultPass {
        ReflectPass( SceneNode& node, RenderQueue Queue ) : 
            DefaultPass(Queue), m_Node( node ) {}
        bool Enable( SceneNode& node ) override {
            return &m_Node != &node;
        }
        bool Visit( SceneNode& node ) override;
        SceneNode& m_Node;
    };

    OutlinePass m_OutlinePass;
    GraphicsPSO m_MirrorPSO;
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
    // Stencil mask
    context.ClearStencil( g_SceneDepthBuffer, 0 );
    Matrix4 model = node.GetTransform();
    context.SetDynamicConstantBufferView( 2, sizeof( model ), &model, { kBindVertex } );
    RenderPass pass( kRenderQueueReflectStencil );
    pass.SetRenderArgs( *m_RenderArgs );
    node.Render( context, pass );

    ReflectPass reflectPass( node, kRenderQueueReflectOpaque );
    m_Scene->Render( reflectPass, *m_RenderArgs );

    // TODO: Render mirror with alpha-blend

    return true;
}

bool Forward::ReflectPass::Visit( SceneNode& node ) 
{
    if (!Enable( node ))
        return false;
    if (m_RenderArgs != nullptr)
    {
        GraphicsContext& context = m_RenderArgs->gfxContext;
        Matrix4 modelMatrix = Matrix4::MakeScale( Vector3( 1, -1, 1 ) ) * node.GetTransform();
        context.SetDynamicConstantBufferView( 2, sizeof( modelMatrix ), &modelMatrix, { kBindVertex } );
        node.Render( context, *this );
    }
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
}

void Forward::Render( std::shared_ptr<Scene>& scene, RenderArgs& args )
{
    GraphicsContext& gfxContext = args.gfxContext;
    {
        ScopedTimer( L"Forward Pass", gfxContext );
        gfxContext.ClearColor( g_EmissiveColorBuffer );
        D3D11_RTV_HANDLE rtvs[] = {
            g_SceneColorBuffer.GetRTV(),
            g_EmissiveColorBuffer.GetRTV(),
        };
        gfxContext.SetRenderTargets( _countof( rtvs ), rtvs, g_SceneDepthBuffer.GetDSV() );
        DefaultPass defaultPass;
        scene->Render( defaultPass, args );
    }
    {
        ScopedTimer( L"Outline Pass", gfxContext );
        scene->Render( m_OutlinePass, args );
    }
    {
        ScopedTimer( L"Reflect Pass", gfxContext );
        MirrorPass mirror( scene );
        scene->Render( mirror, args );
    }
}

void Forward::Shutdown( void )
{
}