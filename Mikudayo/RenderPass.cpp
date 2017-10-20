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
    if (!Enable( material ))
        return false;

    // Reserve access pattern to accept > visit
    // then this logic can be extracted in in mesh accept function
    if (m_RenderQueue != kRenderQueueEmpty)
    {
        RenderPipelinePtr pso = material.GetPipeline( m_RenderQueue );
        if (pso == nullptr)
            return false;
        m_RenderArgs->gfxContext.SetPipelineState( *pso );
    }
    material.Bind( m_RenderArgs->gfxContext );
    return true;
}

bool RenderPass::Visit( SceneNode& node )
{
    if (m_RenderArgs != nullptr)
        node.Render( m_RenderArgs->gfxContext, *this );
    return true;
}