#pragma once

#include <array>
#include <memory>

enum RenderQueue
{
    kRenderQueueDepth = 0,
    kRenderQueueOpaque,
    kRenderQueueOpaqueTwoSided,
    kRenderQueueTransparent,
    kRenderQueueTransparentTwoSided,
    kRenderQueueDeferredGBuffer,
    kRenderQueueDeferredFinal,
    kRenderQueueSkinning,
    kRenderQueueOutline,
    kRenderQueueShadow,
    kRenderQueueSkydome,
    kRenderQueueReflectOpaque,
    kRenderQueueReflectOpaqueTwoSided,
    kRenderQueueReflectTransparent,
    kRenderQueueReflectTransparentTwoSided,
    kRenderQueueReflectorOpaque,
    kRenderQueueReflectorOpaqueTwoSided,
    kRenderQueueReflectorTransparent,
    kRenderQueueReflectorTransparentTwoSided,
    kRenderQueueEmpty,
    kRenderQueueMax
};

typedef std::shared_ptr<class GraphicsPSO> RenderPipelinePtr;
typedef std::array<RenderPipelinePtr, kRenderQueueMax> RenderPipelineList;
static const RenderPipelineList PipelineListEmpty;

void AutoFillPSO( RenderPipelinePtr& basePSO, uint32_t baseIndex, RenderPipelineList& list );

