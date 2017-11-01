#pragma once

#include <vector>
#include <memory>

struct InputDesc;

using FxTechniquePtr = std::shared_ptr<struct FxTechnique>;
using FxContainerPtr = std::shared_ptr<class FxContainer>;
using FxSamplerPtr = std::shared_ptr<struct FxSampler>;

class FxTechniqueSet
{
public:

    FxTechniqueSet(const FxContainerPtr& Container);

    FxTechniquePtr RequestTechnique(const std::string& TechName, const std::vector<InputDesc>& Desc);

protected:

    FxContainerPtr m_Container;
    std::vector<FxSamplerPtr> m_Sampler;
    std::map<std::pair<size_t, std::string>, FxTechniquePtr> m_Technique;
};

