#pragma once

#include "Model.h"
#include "SceneNode.h"

class BaseModel : public IModel, public SceneNode
{
public:

    struct Material
    {
        Vector3 diffuse;
        Vector3 specular;
        Vector3 ambient;
        Vector3 emissive;
        Vector3 transparent; // light passing through a transparent surface is multiplied by this filter color
        float opacity;
        float shininess; // specular exponent
        float specularStrength; // multiplier on top of specular color

        enum { kDiffuse, kSpecular, kEmissive, kNormal, kLightmap, kReflection, kTexCount = 6};
        std::wstring name;
    };

    struct Header
    {
        uint32_t meshCount;
        uint32_t materialCount;
        uint32_t vertexDataByteSize;
        uint32_t indexDataByteSize;
        uint32_t vertexDataByteSizeDepth;
        BoundingBox boundingBox;
    };
    Header m_Header;

    BaseModel();

    void Clear() override;
    bool Load( const ModelInfo& info ) override;

protected:

    EModelType m_Type;
    std::wstring m_Name;
    std::wstring m_File;
    std::wstring m_DefaultShader;
};