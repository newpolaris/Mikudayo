//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author(s):   Alex Nankervis
//              James Stanard
//

#pragma once

#include "VectorMath.h"
#include "TextureManager.h"
#include "GpuBuffer.h"
#include "BaseModel.h"

using namespace Math;

class IColorBuffer;
class MiniModel : public BaseModel
{
public:

    MiniModel();
    ~MiniModel();

    virtual void Clear() override;

    enum
    {
        attrib_mask_0 = (1 << 0),
        attrib_mask_1 = (1 << 1),
        attrib_mask_2 = (1 << 2),
        attrib_mask_3 = (1 << 3),
        attrib_mask_4 = (1 << 4),
        attrib_mask_5 = (1 << 5),
        attrib_mask_6 = (1 << 6),
        attrib_mask_7 = (1 << 7),
        attrib_mask_8 = (1 << 8),
        attrib_mask_9 = (1 << 9),
        attrib_mask_10 = (1 << 10),
        attrib_mask_11 = (1 << 11),
        attrib_mask_12 = (1 << 12),
        attrib_mask_13 = (1 << 13),
        attrib_mask_14 = (1 << 14),
        attrib_mask_15 = (1 << 15),

        // friendly name aliases
        attrib_mask_position = attrib_mask_0,
        attrib_mask_texcoord0 = attrib_mask_1,
        attrib_mask_normal = attrib_mask_2,
        attrib_mask_tangent = attrib_mask_3,
        attrib_mask_bitangent = attrib_mask_4,
    };

    enum
    {
        attrib_0 = 0,
        attrib_1 = 1,
        attrib_2 = 2,
        attrib_3 = 3,
        attrib_4 = 4,
        attrib_5 = 5,
        attrib_6 = 6,
        attrib_7 = 7,
        attrib_8 = 8,
        attrib_9 = 9,
        attrib_10 = 10,
        attrib_11 = 11,
        attrib_12 = 12,
        attrib_13 = 13,
        attrib_14 = 14,
        attrib_15 = 15,

        // friendly name aliases
        attrib_position = attrib_0,
        attrib_texcoord0 = attrib_1,
        attrib_normal = attrib_2,
        attrib_tangent = attrib_3,
        attrib_bitangent = attrib_4,

        maxAttribs = 16
    };

    enum
    {
        attrib_format_none = 0,
        attrib_format_ubyte,
        attrib_format_byte,
        attrib_format_ushort,
        attrib_format_short,
        attrib_format_float,

        attrib_formats
    };

    struct BoundingBox
    {
        Vector3 min;
        Vector3 max;
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

	struct Attrib
	{
		uint16_t offset; // byte offset from the start of the vertex
		uint16_t normalized; // if true, integer formats are interpreted as [-1, 1] or [0, 1]
		uint16_t components; // 1-4
		uint16_t format;
	};
	struct Mesh
    {
        BoundingBox boundingBox;

        unsigned int materialIndex;

        unsigned int attribsEnabled;
        unsigned int attribsEnabledDepth;
        unsigned int vertexStride;
        unsigned int vertexStrideDepth;
        Attrib attrib[maxAttribs];
        Attrib attribDepth[maxAttribs];

        unsigned int vertexDataByteOffset;
        unsigned int vertexCount;
        unsigned int indexDataByteOffset;
        unsigned int indexCount;

        unsigned int vertexDataByteOffsetDepth;
        unsigned int vertexCountDepth;
    };
    Mesh *m_pMesh;

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

        enum {maxTexPath = 128};
        enum {texCount = 6};
        char texDiffusePath[maxTexPath];
        char texSpecularPath[maxTexPath];
        char texEmissivePath[maxTexPath];
        char texNormalPath[maxTexPath];
        char texLightmapPath[maxTexPath];
        char texReflectionPath[maxTexPath];

        enum {maxMaterialName = 128};
        char name[maxMaterialName];
    };
    Material *m_pMaterial;

    unsigned char *m_pVertexData;
    unsigned char *m_pIndexData;

    // optimized for depth-only rendering
    unsigned char *m_pVertexDataDepth;
    unsigned char *m_pIndexDataDepth;
    uint32_t m_VertexStride;
    uint32_t m_VertexStrideDepth;

    Math::BoundingBox GetBoundingBox() const override;

    virtual bool Load( const ModelInfo& info ) override;

protected:

    virtual bool Load( const char* filename );
    const IColorBuffer* LoadTexture( const std::string& name, bool bSRGB );

	bool ImportH3D(const char *filename);
	bool SaveH3D(const char *filename) const;

	void ComputeMeshBoundingBox(unsigned int meshIndex, BoundingBox &bbox) const;
	void ComputeGlobalBoundingBox(BoundingBox &bbox) const;
	void ComputeAllBoundingBoxes();
};

inline Math::BoundingBox MiniModel::GetBoundingBox() const
{
    return m_Transform * Math::BoundingBox(m_Header.boundingBox.min, m_Header.boundingBox.max);
}

