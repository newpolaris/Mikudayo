#pragma once

#include <string>
#include "SamplerManager.h"

struct FxSampler
{
    SamplerDesc desc;
    D3D11_SAMPLER_HANDLE handle = nullptr;
    uint32_t slot = 0;
};
class GraphicsPSO;
class GraphicsContext;
class FxContainer
{
public:

    FxContainer( const std::wstring& FilePath );

    bool Load();
    std::vector<GraphicsPSO> FindTechnique( const std::string& TechName ) const;
    std::vector<FxSampler> GetSampler() const;

protected:

    std::wstring m_FilePath;   
    std::vector<FxSampler> m_Sampler;
    std::map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> m_ShaderByteCode;
    std::map<std::string, std::vector<GraphicsPSO>> m_Technique;
};