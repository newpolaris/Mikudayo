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

    BasicPass m_basicPass;
    TwoSidedPass m_twoSidedPass;
    OutlinePass m_OutlinePass;
};

TransparentPass m_TransparentPass;

void Forward::Initialize( void )
{
}

void Forward::Render( std::shared_ptr<Scene>& scene, RenderArgs& args )
{
    GraphicsContext& gfxContext = args.gfxContext;

    {
        ScopedTimer _prof( L"Forward Pass", gfxContext );
        gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
        scene->Render( m_basicPass, args );
        scene->Render( m_twoSidedPass, args );
    }
    {
        ScopedTimer _prof( L"Outline Pass", gfxContext );
        scene->Render( m_OutlinePass, args );
    }
}

void Forward::Shutdown( void )
{
}