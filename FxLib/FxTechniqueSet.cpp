#include "stdafx.h"
#include "FxTechniqueSet.h"
#include "FxTechnique.h"
#include "FxContainer.h"
#include "InputLayout.h"
#include <boost/functional/hash.hpp>

FxTechniqueSet::FxTechniqueSet(const FxContainerPtr& Container) :
    m_Container(Container)
{
    std::vector<FxSampler> sampler = m_Container->GetSampler();
    for (auto& s : sampler)
    {
        auto ptr = std::make_shared<FxSampler>(s);
        if (ptr)
            ptr->handle = ptr->desc.CreateDescriptor();
        m_Sampler.push_back(ptr);
    }
}

FxTechniquePtr FxTechniqueSet::RequestTechnique(
    const std::string& TechName,
    const std::vector<InputDesc>& Desc)
{
    if (!m_Container)
        return nullptr;
    size_t hashCode = boost::hash_value(Desc);
    auto& tech = m_Technique[{ hashCode, TechName }];
    if (!tech)
    {
        auto pso = m_Container->FindTechnique(TechName);
        if (pso.size() == 0)
            return nullptr;
        auto technique = std::make_shared<FxTechnique>(m_Sampler);
        if (!technique->Load(pso, Desc))
            return nullptr;
        tech = technique;
    }
    return tech;
}