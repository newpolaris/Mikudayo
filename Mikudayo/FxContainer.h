#pragma once

#include <string>

struct FxSampler
{
    uint32_t slot;
    D3D11_SAMPLER_HANDLE handle;
};
class GraphicsPSO;
class GraphicsContext;
class FxContainer
{
public:

    FxContainer( const std::wstring& FileName );

    bool Load();
    uint32_t FindTechnique( const std::string& TechName ) const;
    void SetPass( GraphicsContext& Context, const std::string& TechName, uint32_t Pass );
    void SetSampler( GraphicsContext& Context );

protected:

    std::wstring m_FilePath;   
    std::vector<FxSampler> m_Sampler;
    std::map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> m_ShaderByteCode;
    std::map<std::string, std::vector<std::shared_ptr<GraphicsPSO>>> m_Technique;
};