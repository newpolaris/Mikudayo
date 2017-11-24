#pragma once

using namespace Math;

enum ModelType
{
    kModelDefault,
    kModelPMD,
    kModelPMX,
    kModelSkydome,
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
    std::wstring DefaultShader;
    ModelType Type = kModelUnknown;
    CustomShaderInfo Shader;
    Math::AffineTransform Transform;
};

class IModel
{
public:

    virtual ~IModel();

    virtual void Clear();
    virtual bool Load( const ModelInfo& info ) = 0;

protected:
};

inline IModel::~IModel()
{
    Clear();
}

inline void IModel::Clear()
{
}
