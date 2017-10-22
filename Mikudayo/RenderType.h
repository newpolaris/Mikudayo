#pragma once

#include <array>
#include <memory>

enum RenderQueue
{
    kRenderQueueOpaque,
    kRenderQueueTransparent,
    kRenderQueueTransparentTwoSided,
    kRenderQueueDeferredGBuffer,
    kRenderQueueDeferredFinal,
    kRenderQueueOutline,
    kRenderQueueShadow,
    kRenderQueueEmpty,
    kRenderQueueMax
};

typedef std::shared_ptr<class GraphicsPSO> RenderPipelinePtr;
typedef std::array<RenderPipelinePtr, kRenderQueueMax> RenderPipelineList;

