#include "stdafx.h"
#include "RenderType.h"
#include "PipelineState.h"
#include "GraphicsCore.h"

void AutoFillPSO( RenderPipelinePtr& basePSO, uint32_t baseIndex, RenderPipelineList& list )
{
    using namespace Graphics;

    RenderPipelinePtr baseTowSidedPSO, transparentPSO, transparentTwoSidedPSO;

    D3D11_RASTERIZER_DESC twosided = basePSO->GetRasterizerState();
    twosided.CullMode = D3D11_CULL_NONE;

    baseTowSidedPSO = std::make_shared<GraphicsPSO>();
    *baseTowSidedPSO = *basePSO;
    baseTowSidedPSO->SetRasterizerState( twosided );
    baseTowSidedPSO->Finalize();

    transparentPSO = std::make_shared<GraphicsPSO>();
    *transparentPSO = *basePSO;
    transparentPSO->SetBlendState( BlendTraditional );
    transparentPSO->Finalize();

    transparentTwoSidedPSO = std::make_shared<GraphicsPSO>();
    *transparentTwoSidedPSO = *transparentPSO;
    transparentTwoSidedPSO->SetRasterizerState( twosided );
    transparentTwoSidedPSO->Finalize();

    list[baseIndex] = basePSO;
    list[baseIndex + 1] = baseTowSidedPSO;
    list[baseIndex + 2] = transparentPSO;
    list[baseIndex + 3] = transparentTwoSidedPSO;
}
