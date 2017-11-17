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

using namespace Math;
using namespace Graphics;

namespace Forward
{
    class DefaultPass : public RenderPass
    {
    public:

        DefaultPass( RenderQueue Queue = kRenderQueueOpaque ) : RenderPass( Queue ), m_BaseQueue( Queue ) {}
        uint32_t GetOffset( IMaterial& material ) {
            int index = 0;
            if (!material.IsTransparent())
                index = !material.IsTwoSided() ? kRenderQueueOpaque : kRenderQueueOpaqueTwoSided;
            else
                index = !material.IsTwoSided() ? kRenderQueueTransparent : kRenderQueueTransparentTwoSided;
            return index - kRenderQueueOpaque;
        }
        virtual bool Enable( IMaterial& material ) {
            RenderQueue target = RenderQueue(m_BaseQueue + GetOffset( material ));
            SetRenderQueue( target );
            return true;
        }
        RenderQueue m_BaseQueue;
    };

    class MirrorPass : public DefaultPass 
    {
    public:

        MirrorPass(ScenePtr& scene) : 
            DefaultPass( kRenderQueueReflectorOpaque ), m_Scene(scene) {}

        bool Enable( SceneNode& node ) override;
        bool Visit( SceneNode& node ) override;

        ScenePtr& m_Scene;
    };

    struct ReflectedPass : public DefaultPass {
        ReflectedPass( SceneNode& node, RenderQueue Queue ) : 
            DefaultPass(Queue), m_Node( node ) 
        {
            Vector3 normal = node.GetTransform().Get3x3() * Vector3( 0, 0, 1 );
            Vector3 position = Vector3( node.GetTransform().GetW() );
            m_ReflectionPlane = Vector4( XMPlaneFromPointNormal( position, normal ) );
            m_ReflectMatrix = Matrix4( XMMatrixReflect( m_ReflectionPlane ) );
        }
        bool Enable( SceneNode& node ) override {
            return &m_Node != &node;
        }
        bool Visit( SceneNode& node ) override;
        SceneNode& m_Node;
        Vector4 m_ReflectionPlane;
        Matrix4 m_ReflectMatrix;
    };

    OutlinePass m_OutlinePass;
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

    // Render with clip enabled shader
    ReflectedPass reflectPass( node, kRenderQueueReflectOpaque );
    m_Scene->Render( reflectPass, *m_RenderArgs );

    D3D11_RTV_HANDLE rtvs2[] = {
        g_SceneColorMSBuffer.GetRTV(),
        g_EmissiveColorMSBuffer.GetRTV(),
    };
    context.SetRenderTargets( _countof( rtvs2 ), rtvs2, g_SceneDepthMSBuffer.GetDSV() );

    // Render mirror with alpha-blend
    Matrix4 model = node.GetTransform();
    context.SetDynamicConstantBufferView( 2, sizeof( model ), &model, { kBindVertex } );
    context.SetDynamicDescriptor( 63, g_ReflectColorBuffer.GetSRV(), { kBindPixel } );
    context.SetDynamicDescriptor( 64, g_ReflectEmissiveBuffer.GetSRV(), { kBindPixel } );
    node.Render( context, *this );

    return true;
}

bool Forward::ReflectedPass::Visit( SceneNode& node ) 
{
    if (!Enable( node ))
        return false;
    if (m_RenderArgs != nullptr)
    {
        GraphicsContext& context = m_RenderArgs->gfxContext;
        Matrix4 modelMatrix = m_ReflectMatrix * node.GetTransform();
        context.SetDynamicConstantBufferView( 2, sizeof( modelMatrix ), &modelMatrix, { kBindVertex } );
        context.SetDynamicConstantBufferView( 11, sizeof( m_ReflectionPlane ), &m_ReflectionPlane, { kBindPixel } );
        node.Render( context, *this );
    }
    return true;
}

void Forward::Initialize( void )
{
}

void Forward::Render( std::shared_ptr<Scene>& scene, RenderArgs& args )
{
    GraphicsContext& gfxContext = args.gfxContext;
    {
        ScopedTimer _forward( L"Forward Pass", gfxContext );
        D3D11_RTV_HANDLE rtvs[] = {
            g_SceneColorMSBuffer.GetRTV(),
            g_EmissiveColorMSBuffer.GetRTV(),
        };
        gfxContext.SetRenderTargets( _countof( rtvs ), rtvs, g_SceneDepthMSBuffer.GetDSV() );
        DefaultPass defaultPass;
        scene->Render( defaultPass, args );
    }
    {
        ScopedTimer _outline( L"Outline Pass", gfxContext );
        scene->Render( m_OutlinePass, args );
    }
    {
        ScopedTimer _reflect( L"Reflect Pass", gfxContext );
        MirrorPass mirror( scene );
        scene->Render( mirror, args );
    }

    D3D11_RTV_HANDLE rtvs[] = {
        g_SceneColorMSBuffer.GetRTV(),
        g_EmissiveColorMSBuffer.GetRTV(),
    };
    gfxContext.SetRenderTargets( _countof( rtvs ), rtvs, g_SceneDepthMSBuffer.GetDSV() );
}

void Forward::Shutdown( void )
{
}