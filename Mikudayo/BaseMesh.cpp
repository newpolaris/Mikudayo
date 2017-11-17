#include "stdafx.h"
#include "BaseMesh.h"

using namespace Math;

BaseMesh::BaseMesh() :
    materialIndex(0),
    attribsEnabled(0),
    attribsEnabledDepth(0),
    vertexStride(0),
    vertexStrideDepth(0),
    startIndex(0),
    baseVertex(0),
    vertexDataByteOffset(0),
    vertexCount(0),
    indexDataByteOffset(0),
    vertexDataByteOffsetDepth(0),
    vertexCountDepth(0)
{
}
