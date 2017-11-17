#pragma once

#include "Mesh.h"
#include "BaseModelTypes.h"
#include "Math/BoundingBox.h"

struct BaseMesh : IMesh
{
    BaseMesh();

    Math::BoundingBox boundingBox;
    MaterialPtr material;

    unsigned int materialIndex;
    unsigned int attribsEnabled;
    unsigned int attribsEnabledDepth;
    unsigned int vertexStride;
    unsigned int vertexStrideDepth;
    // Attrib attrib[maxAttribs];
    // Attrib attribDepth[maxAttribs];
    uint32_t startIndex;
    uint32_t baseVertex;
    unsigned int vertexDataByteOffset;
    unsigned int vertexCount;
    unsigned int indexDataByteOffset;
    unsigned int indexCount;

    unsigned int vertexDataByteOffsetDepth;
    unsigned int vertexCountDepth;
};

