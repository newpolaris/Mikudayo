#include "stdafx.h"
#include "RenderPass.h"
#include "RenderArgs.h"
#include "SceneNode.h"
#include "Material.h"
#include "Mesh.h"
#include "Math/BoundingFrustum.h"

using namespace Math;

RenderPass::RenderPass( RenderQueue Queue ) : 
    m_RenderQueue( Queue )
{
}

bool RenderPass::Enable( IMesh& mesh )
{
    bool bEnable = true;
    if (m_RenderArgs)
    {
        const BoundingFrustum& frustum = m_RenderArgs->m_Camera.GetWorldSpaceFrustum();
        bEnable = mesh.IsIntersect( frustum );
    }
    return bEnable;
}

bool RenderPass::Enable( IMesh& mesh, SceneNode& node )
{
    bool bEnable = true;
    if (!node.IsDynamic())
        bEnable = Enable( mesh );
    return bEnable;
}

bool RenderPass::Enable( SceneNode& node )
{
    bool bEnable = true;
    if (m_RenderArgs)
    {
        const BoundingFrustum& frustum = m_RenderArgs->m_Camera.GetWorldSpaceFrustum();
        const Math::BoundingBox& box = node.GetBoundingBox();
		// BoundingBox default constructor (Skydome)
        bEnable |= box.IsValid() ? frustum.IntersectBox( box ) : true;
    }
    bEnable &= node.GetType() != kSceneMirror;
    return bEnable;
}

void RenderPass::SetRenderArgs( RenderArgs& args )
{
    m_RenderArgs = &args;
}

void RenderPass::SetRenderQueue( RenderQueue queue )
{
    m_RenderQueue = queue;
}

bool RenderPass::Visit( IMesh& mesh )
{
    if (!Enable( mesh ))
        return false;
    return true;
}

bool RenderPass::Visit( IMesh& mesh, SceneNode& node )
{
    if (!Enable( mesh, node ))
        return false;
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
    if (!Enable( node ))
        return false;
    if (m_RenderArgs != nullptr)
    {
        GraphicsContext& context = m_RenderArgs->gfxContext;
        Matrix4 modelMatrix = node.GetTransform();
        context.SetDynamicConstantBufferView(2, sizeof(modelMatrix), &modelMatrix, { kBindVertex });
        node.Render( context, *this );
    }
    return true;
}
