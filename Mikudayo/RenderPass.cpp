#include "stdafx.h"
#include "RenderPass.h"
#include "RenderArgs.h"
#include "SceneNode.h"
#include "Material.h"

RenderPass::RenderPass( RenderQueue Queue ) : 
    m_RenderQueue( Queue )
{
}

void RenderPass::SetRenderArgs( RenderArgs& args )
{
    m_RenderArgs = &args;
}

bool RenderPass::Visit( IMaterial& material ) 
{ 
    if (m_RenderQueue != kRenderQueueEmpty)
    {
        RenderPipelinePtr pso = material.GetPipeline( m_RenderQueue );
        if (pso != nullptr)
            m_RenderArgs->gfxContext.SetPipelineState( *pso );
    }
    return true; 
}

bool RenderPass::Visit( SceneNode& node )
{
    if (m_RenderArgs != nullptr)
        node.Render( m_RenderArgs->gfxContext, *this );
    return true;
}