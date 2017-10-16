#pragma once

using namespace Math;

enum EModelType
{
    kModelPMX
};

struct TextureInfo
{
    uint32_t Slot;
    std::wstring Path;
};

struct CustomShaderInfo
{
    std::wstring Name;
    std::vector<std::wstring> MaterialNames;
    std::vector<TextureInfo> Textures;
};

struct ModelInfo
{
    EModelType Type;
    std::wstring Name;
    std::wstring File;
    std::wstring DefaultShader = L"Default";
    CustomShaderInfo Shader;
};

class Model
{
public:

    Model();
    ~Model();
    void Clear();

    virtual bool Load( const ModelInfo& Info ) = 0;

protected:
};