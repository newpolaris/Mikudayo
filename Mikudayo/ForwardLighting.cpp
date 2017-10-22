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

using namespace Math;
using namespace Graphics;

namespace Forward
{
    OutlinePass m_OutlinePass;
    TransparentPass m_TransparentPass;
}

void Forward::Initialize( void )
{
}

void Forward::Render( std::shared_ptr<Scene>& scene, RenderArgs& args )
{
    GraphicsContext& gfxContext = args.gfxContext;

    {
        ScopedTimer _prof( L"Forward Pass", gfxContext );
        gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
        OpaquePass opaque( kRenderQueueOpaque );
        scene->Render( opaque, args );
        scene->Render( m_TransparentPass, args );
    }
    {
        ScopedTimer _prof( L"Outline Pass", gfxContext );
        scene->Render( m_OutlinePass, args );
    }
}

void Forward::Shutdown( void )
{
}