#include "stdafx.h"
#include "RenderType.h"
#include "PipelineState.h"
#include "GraphicsCore.h"

void AutoFillPSO( RenderPipelinePtr& basePSO, uint32_t baseIndex, RenderPipelineList& list )
{
    using namespace Graphics;

    RenderPipelinePtr baseTowSidedPSO, transparentPSO, transparentTwoSidedPSO;

    baseTowSidedPSO = std::make_shared<GraphicsPSO>();
    *baseTowSidedPSO = *basePSO;
    baseTowSidedPSO->SetRasterizerState( RasterizerTwoSided );
    baseTowSidedPSO->Finalize();

    transparentPSO = std::make_shared<GraphicsPSO>();
    *transparentPSO = *basePSO;
    transparentPSO->SetBlendState( BlendTraditional );
    transparentPSO->Finalize();

    transparentTwoSidedPSO = std::make_shared<GraphicsPSO>();
    *transparentTwoSidedPSO = *transparentPSO;
    transparentTwoSidedPSO->SetRasterizerState( RasterizerTwoSided );
    transparentTwoSidedPSO->Finalize();

    list[baseIndex] = basePSO;
    list[baseIndex + 1] = baseTowSidedPSO;
    list[baseIndex + 2] = transparentPSO;
    list[baseIndex + 3] = transparentTwoSidedPSO;
}
