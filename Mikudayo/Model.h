#pragma once

using namespace Math;

enum ModelType
{
    kModelNone,
    kModelDefault,
    kModelPMX,
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
    std::wstring ModelFile;
    std::wstring MotionFile;
    std::wstring DefaultShader = L"Default";
    CustomShaderInfo Shader;
};

class IModel
{
public:

    IModel();
    virtual ~IModel();

    virtual void Clear();
    virtual bool Load( const ModelInfo& info ) = 0;

protected:
};