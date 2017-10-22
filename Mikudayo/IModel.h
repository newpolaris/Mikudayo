#pragma once

using namespace Math;

enum ModelType
{
    kModelDefault,
    kModelPMX,
    kModelUnknown,
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
    ModelType Type = kModelUnknown;
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

inline IModel::IModel()
{
}

inline IModel::~IModel()
{
    Clear();
}

inline void IModel::Clear()
{
}
