#include "stdafx.h"
#include "FxTechnique.h"
#include "InputLayout.h"

FxTechnique::FxTechnique(const std::vector<std::shared_ptr<FxSampler>>& Samp) :
    Sampler(Samp)
{
}

bool FxTechnique::Load(const std::vector<GraphicsPSO>& PSO, const std::vector<InputDesc>& Desc)
{
    for (auto& pso : PSO)
    {
        auto ptr = std::make_shared<GraphicsPSO>(pso);
        ptr->SetInputLayout((UINT)Desc.size(), Desc.data());
        ptr->Finalize();
        Pass.push_back(std::move(ptr));
    }
    return true;
}

void FxTechnique::Render(GraphicsContext& Context, std::function<void(GraphicsContext&)> Call)
{
    for (auto& pass : Pass)
    {
        Context.SetPipelineState(*pass);
        Call(Context);
    }
}
