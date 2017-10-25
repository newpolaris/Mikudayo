#pragma once

#include <array>
#include <memory>

enum RenderQueue
{
    kRenderQueueOpaque = 0,
    kRenderQueueOpaqueTwoSided,
    kRenderQueueTransparent,
    kRenderQueueTransparentTwoSided,
    kRenderQueueDeferredGBuffer,
    kRenderQueueDeferredFinal,
    kRenderQueueOutline,
    kRenderQueueShadow,
    kRenderQueueReflectStencil,
    kRenderQueueReflectOpaque,
    kRenderQueueReflectOpaqueTwoSided,
    kRenderQueueReflectTransparent,
    kRenderQueueReflectTransparentTwoSided,
    kRenderQueueEmpty,
    kRenderQueueMax
};

typedef std::shared_ptr<class GraphicsPSO> RenderPipelinePtr;
typedef std::array<RenderPipelinePtr, kRenderQueueMax> RenderPipelineList;

