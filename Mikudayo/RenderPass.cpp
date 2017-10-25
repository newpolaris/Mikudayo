#include "stdafx.h"
#include "RenderPass.h"
#include "RenderArgs.h"
#include "SceneNode.h"
#include "Material.h"

using namespace Math;

RenderPass::RenderPass( RenderQueue Queue ) : 
    m_RenderQueue( Queue )
{
}

void RenderPass::SetRenderArgs( RenderArgs& args )
{
    m_RenderArgs = &args;
}

bool RenderPass::Visit( IMesh& )
{
    return true;
}

bool RenderPass::Visit( IMaterial& material ) 
{ 
    if (!Enable( material ))
        return false;

    // Reserve access pattern to accept > visit
    // then this logic can be extracted in in mesh accept function
    if (m_RenderQueue != kRenderQueueEmpty && m_RenderArgs)
    {
        RenderPipelinePtr pso = material.GetPipeline( m_RenderQueue );
        if (pso == nullptr)
            return false;
        m_RenderArgs->gfxContext.SetPipelineState( *pso );
    }
    if (m_RenderArgs)
        material.Bind( m_RenderArgs->gfxContext );
    return true;
}

bool RenderPass::Visit( SceneNode& node )
{
    if (m_RenderArgs != nullptr)
    {
        Matrix4 modelTransform = node.GetTransform();
        Matrix4 modelMatrix = m_RenderArgs->m_ModelMatrix * modelTransform;
        GraphicsContext& context = m_RenderArgs->gfxContext;
        context.SetDynamicConstantBufferView(2, sizeof(modelMatrix), &modelMatrix, { kBindVertex });
        node.Render( context, *this );

    }
    return true;
}