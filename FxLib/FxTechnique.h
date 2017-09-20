#pragma once

#include <memory>
#include <vector>

struct FxSampler;
class GraphicsPSO;

struct FxTechnique
{
public:

    FxTechnique(const std::vector<std::shared_ptr<FxSampler>>& Samp);

    bool Load(const std::vector<GraphicsPSO>& PSO, const std::vector<InputDesc>& Desc);
    void Render(GraphicsContext& Context, std::function<void(GraphicsContext&)> Call);

    std::vector<std::shared_ptr<FxSampler>> Sampler;
    std::vector<std::shared_ptr<GraphicsPSO>> Pass;
};

