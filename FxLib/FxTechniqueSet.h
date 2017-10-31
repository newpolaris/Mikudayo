#pragma once

#include <vector>

class FxContainer;
struct FxTechnique;
struct FxSampler;
struct InputDesc;

class FxTechniqueSet
{
public:

    FxTechniqueSet(const std::shared_ptr<FxContainer>& Container);

    std::shared_ptr<FxTechnique> RequestTechnique(const std::string& TechName, const std::vector<InputDesc>& Desc);

protected:

    std::shared_ptr<FxContainer> m_Container;
    std::vector<std::shared_ptr<FxSampler>> m_Sampler;
    std::map<std::pair<size_t, std::string>, std::shared_ptr<FxTechnique>> m_Technique;
};

