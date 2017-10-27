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
    kRenderQueueOutline,
    kRenderQueueShadow,
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

void AutoFillPSO( RenderPipelinePtr& basePSO, uint32_t baseIndex, RenderPipelineList& list );

