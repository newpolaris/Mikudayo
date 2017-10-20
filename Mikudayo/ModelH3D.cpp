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
// Author:  Alex Nankervis
//

#include "stdafx.h"
#include "MiniModel.h"
#include "Utility.h"
#include "TextureManager.h"
#include "GraphicsCore.h"
#include "CommandContext.h"
#include <stdio.h>

bool MiniModel::ImportH3D(const char *filename)
{
    FILE *file = nullptr;
    if (0 != fopen_s(&file, filename, "rb"))
        return false;

    bool ok = false;

    if (1 != fread(&m_Header, sizeof(Header), 1, file)) goto h3d_load_fail;

    m_pMesh = new Mesh [m_Header.meshCount];
    m_pMaterial = new Material [m_Header.materialCount];

    if (m_Header.meshCount > 0)
        if (1 != fread(m_pMesh, sizeof(Mesh) * m_Header.meshCount, 1, file)) goto h3d_load_fail;
    if (m_Header.materialCount > 0)
        if (1 != fread(m_pMaterial, sizeof(Material) * m_Header.materialCount, 1, file)) goto h3d_load_fail;

    m_VertexStride = m_pMesh[0].vertexStride;
    m_VertexStrideDepth = m_pMesh[0].vertexStrideDepth;
#if _DEBUG
    for (uint32_t meshIndex = 1; meshIndex < m_Header.meshCount; ++meshIndex)
    {
        const Mesh& mesh = m_pMesh[meshIndex];
        ASSERT(mesh.vertexStride == m_VertexStride);
        ASSERT(mesh.vertexStrideDepth == m_VertexStrideDepth);
    }
    for (uint32_t meshIndex = 0; meshIndex < m_Header.meshCount; ++meshIndex)
    {
        const Mesh& mesh = m_pMesh[meshIndex];

        ASSERT( mesh.attribsEnabled ==
            (attrib_mask_position | attrib_mask_texcoord0 | attrib_mask_normal | attrib_mask_tangent | attrib_mask_bitangent) );
        ASSERT(mesh.attrib[0].components == 3 && mesh.attrib[0].format == MiniModel::attrib_format_float); // position
        ASSERT(mesh.attrib[1].components == 2 && mesh.attrib[1].format == MiniModel::attrib_format_float); // texcoord0
        ASSERT(mesh.attrib[2].components == 3 && mesh.attrib[2].format == MiniModel::attrib_format_float); // normal
        ASSERT(mesh.attrib[3].components == 3 && mesh.attrib[3].format == MiniModel::attrib_format_float); // tangent
        ASSERT(mesh.attrib[4].components == 3 && mesh.attrib[4].format == MiniModel::attrib_format_float); // bitangent

        ASSERT( mesh.attribsEnabledDepth ==
            (attrib_mask_position) );
        ASSERT(mesh.attrib[0].components == 3 && mesh.attrib[0].format == MiniModel::attrib_format_float); // position
    }
#endif

    m_pVertexData = new unsigned char[ m_Header.vertexDataByteSize ];
    m_pIndexData = new unsigned char[ m_Header.indexDataByteSize ];
    m_pVertexDataDepth = new unsigned char[ m_Header.vertexDataByteSizeDepth ];
    m_pIndexDataDepth = new unsigned char[ m_Header.indexDataByteSize ];

    if (m_Header.vertexDataByteSize > 0)
        if (1 != fread(m_pVertexData, m_Header.vertexDataByteSize, 1, file)) goto h3d_load_fail;
    if (m_Header.indexDataByteSize > 0)
        if (1 != fread(m_pIndexData, m_Header.indexDataByteSize, 1, file)) goto h3d_load_fail;

    if (m_Header.vertexDataByteSizeDepth > 0)
        if (1 != fread(m_pVertexDataDepth, m_Header.vertexDataByteSizeDepth, 1, file)) goto h3d_load_fail;
    if (m_Header.indexDataByteSize > 0)
        if (1 != fread(m_pIndexDataDepth, m_Header.indexDataByteSize, 1, file)) goto h3d_load_fail;

    m_VertexBuffer.Create(L"VertexBuffer", m_Header.vertexDataByteSize / m_VertexStride, m_VertexStride, m_pVertexData);
    m_IndexBuffer.Create(L"IndexBuffer", m_Header.indexDataByteSize / sizeof(uint16_t), sizeof(uint16_t), m_pIndexData);
    delete [] m_pVertexData;
    m_pVertexData = nullptr;
    delete [] m_pIndexData;
    m_pIndexData = nullptr;

    m_VertexBufferDepth.Create(L"VertexBufferDepth", m_Header.vertexDataByteSizeDepth / m_VertexStrideDepth, m_VertexStrideDepth, m_pVertexDataDepth);
    m_IndexBufferDepth.Create(L"IndexBufferDepth", m_Header.indexDataByteSize / sizeof(uint16_t), sizeof(uint16_t), m_pIndexDataDepth);
    delete [] m_pVertexDataDepth;
    m_pVertexDataDepth = nullptr;
    delete [] m_pIndexDataDepth;
    m_pIndexDataDepth = nullptr;

    ok = true;

h3d_load_fail:

    if (EOF == fclose(file))
        ok = false;

    return ok;
}

bool MiniModel::SaveH3D(const char *filename) const
{
    FILE *file = nullptr;
    if (0 != fopen_s(&file, filename, "wb"))
        return false;

    bool ok = false;

    if (1 != fwrite(&m_Header, sizeof(Header), 1, file)) goto h3d_save_fail;

    if (m_Header.meshCount > 0)
        if (1 != fwrite(m_pMesh, sizeof(Mesh) * m_Header.meshCount, 1, file)) goto h3d_save_fail;
    if (m_Header.materialCount > 0)
        if (1 != fwrite(m_pMaterial, sizeof(Material) * m_Header.materialCount, 1, file)) goto h3d_save_fail;

    if (m_Header.vertexDataByteSize > 0)
        if (1 != fwrite(m_pVertexData, m_Header.vertexDataByteSize, 1, file)) goto h3d_save_fail;
    if (m_Header.indexDataByteSize > 0)
        if (1 != fwrite(m_pIndexData, m_Header.indexDataByteSize, 1, file)) goto h3d_save_fail;

    if (m_Header.vertexDataByteSizeDepth > 0)
        if (1 != fwrite(m_pVertexDataDepth, m_Header.vertexDataByteSizeDepth, 1, file)) goto h3d_save_fail;
    if (m_Header.indexDataByteSize > 0)
        if (1 != fwrite(m_pIndexDataDepth, m_Header.indexDataByteSize, 1, file)) goto h3d_save_fail;

    ok = true;

h3d_save_fail:

    if (EOF == fclose(file))
        ok = false;

    return ok;
}