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
// Author(s):	Alex Nankervis
//
#pragma once

#include "stdafx.h"
#include "ModelAssimp.h"
#include "TextUtility.h"
#include "BaseMaterial.h"
#include "BaseMesh.h"
#include <assert.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

const char* AssimpModel::s_FormatString[] =
{
	"none",
	"h3d",
};
static_assert(_countof(AssimpModel::s_FormatString) == AssimpModel::formats, "s_FormatString doesn't match format enum");

int AssimpModel::FormatFromFilename(const char *filename)
{
	const char *p = strrchr(filename, '.');
	if (!p || *p == 0)
		return format_none;

	for (int n = 1; n < formats; n++)
	{
		if (_stricmp(p + 1, s_FormatString[n]) == 0)
			return n;
	}

	return format_none;
}

bool AssimpModel::Load( const ModelInfo& info )
{
    BaseModel::Load( info );

    const std::string name = Utility::MakeStr( m_FileName );
    const char* filename = name.c_str();
    if (!Load(filename))
        return false;

    for (uint32_t i = 0; i < m_Header.materialCount; i++)
    {
        MaterialPtr material = std::make_shared<BaseMaterial>();
        const Material& pMaterial = m_pMaterial[i];
        material->ambient = pMaterial.ambient;
        material->diffuse = pMaterial.diffuse;
        material->specular = pMaterial.specular;
        material->emissive = pMaterial.emissive;
        material->transparent = pMaterial.transparent;
        material->opacity = pMaterial.opacity;
        material->shininess = pMaterial.shininess;
        material->specularStrength = pMaterial.specularStrength;
        const ManagedTexture** MatTextures = material->textures;
        MatTextures[0] = LoadTexture(pMaterial.texDiffusePath, true);
        MatTextures[1] = LoadTexture(pMaterial.texSpecularPath, true);
        MatTextures[2] = LoadTexture(pMaterial.texEmissivePath, true);
        MatTextures[3] = LoadTexture(pMaterial.texNormalPath, false);
        MatTextures[4] = LoadTexture(pMaterial.texLightmapPath, true);
        MatTextures[5] = LoadTexture(pMaterial.texReflectionPath, true);

        m_Materials.push_back( std::move( material ) );
    }

    assert( m_VertexStride > 0 );
    assert( m_VertexStrideDepth > 0 );

    for (uint32_t i = 0; i < m_Header.meshCount; i++)
    {
        MeshPtr mesh = std::make_shared<BaseMesh>();
        mesh->boundingBox = Math::BoundingBox( m_pMesh[i].boundingBox.min, m_pMesh[i].boundingBox.max );
        mesh->materialIndex = m_pMesh[i].materialIndex;
        mesh->material = m_Materials[m_pMesh[i].materialIndex];
        mesh->attribsEnabled = m_pMesh[i].attribsEnabled;
        mesh->attribsEnabledDepth = m_pMesh[i].attribsEnabledDepth;
        mesh->vertexStride = m_pMesh[i].vertexStride;
        mesh->vertexStrideDepth = m_pMesh[i].vertexStrideDepth;
        // Attrib attrib[maxAttribs];
        // Attrib attribDepth[maxAttribs];
        mesh->vertexDataByteOffset = m_pMesh[i].vertexDataByteOffset;
        mesh->vertexCount = m_pMesh[i].vertexCount;
        mesh->indexDataByteOffset = m_pMesh[i].indexDataByteOffset;
        mesh->indexCount = m_pMesh[i].indexCount;
        mesh->vertexDataByteOffsetDepth = m_pMesh[i].vertexDataByteOffsetDepth;
        mesh->vertexCountDepth = m_pMesh[i].vertexCountDepth;
        mesh->startIndex = m_pMesh[i].indexDataByteOffset / sizeof(uint32_t);
        mesh->baseVertex = m_pMesh[i].vertexDataByteOffset / m_VertexStride;

        m_Meshes.push_back( std::move( mesh ) );
    }

    assert( m_Meshes.size() > 0 );

    m_VertexBuffer.Create(L"VertexBuffer", m_Header.vertexDataByteSize / m_VertexStride, m_VertexStride, m_pVertexData);
    m_IndexBuffer.Create(L"IndexBuffer", m_Header.indexDataByteSize / sizeof(uint32_t), sizeof(uint32_t), m_pIndexData);
    delete [] m_pVertexData;
    m_pVertexData = nullptr;
    delete [] m_pIndexData;
    m_pIndexData = nullptr;

    m_VertexBufferDepth.Create(L"VertexBufferDepth", m_Header.vertexDataByteSizeDepth / m_VertexStrideDepth, m_VertexStrideDepth, m_pVertexDataDepth);
    m_IndexBufferDepth.Create(L"IndexBufferDepth", m_Header.indexDataByteSize / sizeof(uint32_t), sizeof(uint32_t), m_pIndexDataDepth);
    delete [] m_pVertexDataDepth;
    m_pVertexDataDepth = nullptr;
    delete [] m_pIndexDataDepth;
    m_pIndexDataDepth = nullptr;

	return true;
}

bool AssimpModel::Load( const char* filename )
{
	Clear();

	int format = FormatFromFilename(filename);

	bool rval = false;
	bool needToOptimize = true;
	switch (format)
	{
	case format_none:
		rval = LoadAssimp(filename);
		break;

	case format_h3d:
		rval = ImportH3D(filename);
		needToOptimize = false;
		break;
	}

	if (!rval)
		return false;

#if 0
	if (needToOptimize)
		Optimize();
#endif

    return true;
}

bool AssimpModel::Save(const char *filename) const
{
	int format = FormatFromFilename(filename);

	bool rval = false;
	switch (format)
	{
	case format_none:
		break;

	case format_h3d:
		rval = SaveH3D(filename);
		break;
	}

	return rval;
}


bool AssimpModel::LoadAssimp(const char* filename)
{
    Assimp::Importer importer;

    // remove unused data
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, 
        aiComponent_COLORS | aiComponent_LIGHTS | aiComponent_CAMERAS);

    // max triangles and vertices per mesh, splits above this threshold
    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, INT_MAX);
    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 0xfffe); // avoid the primitive restart index

    // remove points and lines
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

    const aiScene *scene = importer.ReadFile(filename,
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate |
        aiProcess_RemoveComponent |
        aiProcess_GenSmoothNormals |
        aiProcess_SplitLargeMeshes |
        aiProcess_ValidateDataStructure |
        //aiProcess_ImproveCacheLocality | // handled by optimizePostTransform()
        aiProcess_RemoveRedundantMaterials |
        aiProcess_SortByPType |
        aiProcess_FindInvalidData |
        aiProcess_GenUVCoords |
        aiProcess_TransformUVCoords |
        aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph);

    if (scene == nullptr)
        return false;

    if (scene->HasTextures())
    {
        // embedded textures...
        assert( false );
    }

    if (scene->HasAnimations())
    {
        // todo
    }

    m_Header.materialCount = scene->mNumMaterials;
    m_pMaterial = new Material [m_Header.materialCount];
    memset(m_pMaterial, 0, sizeof(Material) * m_Header.materialCount);
    for (unsigned int materialIndex = 0; materialIndex < scene->mNumMaterials; materialIndex++)
    {
        const aiMaterial *srcMat = scene->mMaterials[materialIndex];
        Material *dstMat = m_pMaterial + materialIndex;

        aiColor3D diffuse(1.0f, 1.0f, 1.0f);
        aiColor3D specular(1.0f, 1.0f, 1.0f);
        aiColor3D ambient(1.0f, 1.0f, 1.0f);
        aiColor3D emissive(0.0f, 0.0f, 0.0f);
        aiColor3D transparent(1.0f, 1.0f, 1.0f);
        float opacity = 1.0f;
        float shininess = 0.0f;
        float specularStrength = 1.0f;
        aiString texDiffusePath;
        aiString texSpecularPath;
        aiString texEmissivePath;
        aiString texNormalPath;
        aiString texLightmapPath;
        aiString texReflectionPath;
        srcMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
        srcMat->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        srcMat->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
        srcMat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
        srcMat->Get(AI_MATKEY_COLOR_TRANSPARENT, transparent);
        srcMat->Get(AI_MATKEY_OPACITY, opacity);
        srcMat->Get(AI_MATKEY_SHININESS, shininess);
        srcMat->Get(AI_MATKEY_SHININESS_STRENGTH, specularStrength);
        srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texDiffusePath);
        srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), texSpecularPath);
        srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE, 0), texEmissivePath);
        srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), texNormalPath);
        srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_LIGHTMAP, 0), texLightmapPath);
        srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_REFLECTION, 0), texReflectionPath);

        dstMat->diffuse = Vector3(diffuse.r, diffuse.g, diffuse.b);
        dstMat->specular = Vector3(specular.r, specular.g, specular.b);
        dstMat->ambient = Vector3(ambient.r, ambient.g, ambient.b);
        dstMat->emissive = Vector3(emissive.r, emissive.g, emissive.b);
        dstMat->transparent = Vector3(transparent.r, transparent.g, transparent.b);
        dstMat->opacity = opacity;
        dstMat->shininess = shininess;
        dstMat->specularStrength = specularStrength;

        strncpy_s(dstMat->texDiffusePath, texDiffusePath.C_Str(), Material::maxTexPath - 1);
        strncpy_s(dstMat->texSpecularPath, texSpecularPath.C_Str(), Material::maxTexPath - 1);
        strncpy_s(dstMat->texEmissivePath, texEmissivePath.C_Str(), Material::maxTexPath - 1);
        strncpy_s(dstMat->texNormalPath, texNormalPath.C_Str(), Material::maxTexPath - 1);
        strncpy_s(dstMat->texLightmapPath, texLightmapPath.C_Str(), Material::maxTexPath - 1);
        strncpy_s(dstMat->texReflectionPath, texReflectionPath.C_Str(), Material::maxTexPath - 1);

        aiString matName;
        srcMat->Get(AI_MATKEY_NAME, matName);
        strncpy_s(dstMat->name, matName.C_Str(), Material::maxMaterialName - 1);
    }

    m_Header.meshCount = scene->mNumMeshes;
    m_pMesh = new Mesh [m_Header.meshCount];
    memset(m_pMesh, 0, sizeof(Mesh) * m_Header.meshCount);
    // first pass, count everything
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
    {
        const aiMesh *srcMesh = scene->mMeshes[meshIndex];
        Mesh *dstMesh = m_pMesh + meshIndex;

        assert(srcMesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

        dstMesh->materialIndex = srcMesh->mMaterialIndex;

        // just store everything as float. Can quantize in Model::optimize()
        dstMesh->attribsEnabled |= attrib_mask_position;
        dstMesh->attrib[attrib_position].offset = dstMesh->vertexStride;
        dstMesh->attrib[attrib_position].normalized = 0;
        dstMesh->attrib[attrib_position].components = 3;
        dstMesh->attrib[attrib_position].format = attrib_format_float;
        dstMesh->vertexStride += sizeof(float) * 3;

        dstMesh->attribsEnabled |= attrib_mask_texcoord0;
        dstMesh->attrib[attrib_texcoord0].offset = dstMesh->vertexStride;
        dstMesh->attrib[attrib_texcoord0].normalized = 0;
        dstMesh->attrib[attrib_texcoord0].components = 2;
        dstMesh->attrib[attrib_texcoord0].format = attrib_format_float;
        dstMesh->vertexStride += sizeof(float) * 2;

        dstMesh->attribsEnabled |= attrib_mask_normal;
        dstMesh->attrib[attrib_normal].offset = dstMesh->vertexStride;
        dstMesh->attrib[attrib_normal].normalized = 0;
        dstMesh->attrib[attrib_normal].components = 3;
        dstMesh->attrib[attrib_normal].format = attrib_format_float;
        dstMesh->vertexStride += sizeof(float) * 3;

        dstMesh->attribsEnabled |= attrib_mask_tangent;
        dstMesh->attrib[attrib_tangent].offset = dstMesh->vertexStride;
        dstMesh->attrib[attrib_tangent].normalized = 0;
        dstMesh->attrib[attrib_tangent].components = 3;
        dstMesh->attrib[attrib_tangent].format = attrib_format_float;
        dstMesh->vertexStride += sizeof(float) * 3;

        dstMesh->attribsEnabled |= attrib_mask_bitangent;
        dstMesh->attrib[attrib_bitangent].offset = dstMesh->vertexStride;
        dstMesh->attrib[attrib_bitangent].normalized = 0;
        dstMesh->attrib[attrib_bitangent].components = 3;
        dstMesh->attrib[attrib_bitangent].format = attrib_format_float;
        dstMesh->vertexStride += sizeof(float) * 3;

        // depth-only
        dstMesh->attribsEnabledDepth |= attrib_mask_position;
        dstMesh->attribDepth[attrib_position].offset = dstMesh->vertexStrideDepth;
        dstMesh->attribDepth[attrib_position].normalized = 0;
        dstMesh->attribDepth[attrib_position].components = 3;
        dstMesh->attribDepth[attrib_position].format = attrib_format_float;
        dstMesh->vertexStrideDepth += sizeof(float) * 3;

        // color rendering
        dstMesh->vertexDataByteOffset = m_Header.vertexDataByteSize;
        dstMesh->vertexCount = srcMesh->mNumVertices;

        dstMesh->indexDataByteOffset = m_Header.indexDataByteSize;
        dstMesh->indexCount = srcMesh->mNumFaces * 3;

        m_Header.vertexDataByteSize += dstMesh->vertexStride * dstMesh->vertexCount;
        m_Header.indexDataByteSize += sizeof(uint32_t) * dstMesh->indexCount;

        // depth-only rendering
        dstMesh->vertexDataByteOffsetDepth = m_Header.vertexDataByteSizeDepth;
        dstMesh->vertexCountDepth = srcMesh->mNumVertices;

        m_Header.vertexDataByteSizeDepth += dstMesh->vertexStrideDepth * dstMesh->vertexCountDepth;
    }

    if (m_Header.meshCount > 0)
    {
        m_VertexStride = m_pMesh[0].vertexStride;
        m_VertexStrideDepth = m_pMesh[0].vertexStrideDepth;
    }

    // allocate storage
    m_pVertexData = new unsigned char [m_Header.vertexDataByteSize];
    m_pIndexData = new unsigned char [m_Header.indexDataByteSize];
    m_pVertexDataDepth = new unsigned char [m_Header.vertexDataByteSizeDepth];
    m_pIndexDataDepth = new unsigned char [m_Header.indexDataByteSize];
    // second pass, fill in vertex and index data
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
    {
        const aiMesh *srcMesh = scene->mMeshes[meshIndex];
        Mesh *dstMesh = m_pMesh + meshIndex;

        float *dstPos = (float*)(m_pVertexData + dstMesh->vertexDataByteOffset + dstMesh->attrib[attrib_position].offset);
        float *dstTexcoord0 = (float*)(m_pVertexData + dstMesh->vertexDataByteOffset + dstMesh->attrib[attrib_texcoord0].offset);
        float *dstNormal = (float*)(m_pVertexData + dstMesh->vertexDataByteOffset + dstMesh->attrib[attrib_normal].offset);
        float *dstTangent = (float*)(m_pVertexData + dstMesh->vertexDataByteOffset + dstMesh->attrib[attrib_tangent].offset);
        float *dstBitangent = (float*)(m_pVertexData + dstMesh->vertexDataByteOffset + dstMesh->attrib[attrib_bitangent].offset);

        float *dstPosDepth = (float*)(m_pVertexDataDepth + dstMesh->vertexDataByteOffsetDepth + dstMesh->attribDepth[attrib_position].offset);

        for (unsigned int v = 0; v < dstMesh->vertexCount; v++)
        {
            if (srcMesh->mVertices)
            {
                dstPos[0] = srcMesh->mVertices[v].x;
                dstPos[1] = srcMesh->mVertices[v].y;
                dstPos[2] = srcMesh->mVertices[v].z;

                dstPosDepth[0] = srcMesh->mVertices[v].x;
                dstPosDepth[1] = srcMesh->mVertices[v].y;
                dstPosDepth[2] = srcMesh->mVertices[v].z;
            }
            else
            {
                // no position? That's kind of bad.
                assert(0);
            }
            dstPos = (float*)((unsigned char*)dstPos + dstMesh->vertexStride);
            dstPosDepth = (float*)((unsigned char*)dstPosDepth + dstMesh->vertexStrideDepth);

            if (srcMesh->mTextureCoords[0])
            {
                dstTexcoord0[0] = srcMesh->mTextureCoords[0][v].x;
                dstTexcoord0[1] = 1-srcMesh->mTextureCoords[0][v].y;
			}
            else
            {
                dstTexcoord0[0] = 0.0f;
                dstTexcoord0[1] = 0.0f;
            }
            dstTexcoord0 = (float*)((unsigned char*)dstTexcoord0 + dstMesh->vertexStride);

            if (srcMesh->mNormals)
            {
                dstNormal[0] = srcMesh->mNormals[v].x;
                dstNormal[1] = srcMesh->mNormals[v].y;
                dstNormal[2] = srcMesh->mNormals[v].z;
            }
            else
            {
                // Assimp should generate normals if they are missing, according to the postprocessing flag specified on load,
                // so we should never get here.
                assert(0);
            }
            dstNormal = (float*)((unsigned char*)dstNormal + dstMesh->vertexStride);

            if (srcMesh->mTangents)
            {
                dstTangent[0] = srcMesh->mTangents[v].x;
                dstTangent[1] = srcMesh->mTangents[v].y;
                dstTangent[2] = srcMesh->mTangents[v].z;
            }
            else
            {
                // TODO: generate tangents/bitangents if missing
                dstTangent[0] = 1.0f;
                dstTangent[1] = 0.0f;
                dstTangent[2] = 0.0f;
            }
            dstTangent = (float*)((unsigned char*)dstTangent + dstMesh->vertexStride);

            if (srcMesh->mBitangents)
            {
                dstBitangent[0] = srcMesh->mBitangents[v].x;
                dstBitangent[1] = srcMesh->mBitangents[v].y;
                dstBitangent[2] = srcMesh->mBitangents[v].z;
            }
            else
            {
                // TODO: generate tangents/bitangents if missing
                dstBitangent[0] = 0.0f;
                dstBitangent[1] = 1.0f;
                dstBitangent[2] = 0.0f;
            }
            dstBitangent = (float*)((unsigned char*)dstBitangent + dstMesh->vertexStride);
        }

        uint32_t *dstIndex = (uint32_t*)(m_pIndexData + dstMesh->indexDataByteOffset);
        uint32_t *dstIndexDepth = (uint32_t*)(m_pIndexDataDepth + dstMesh->indexDataByteOffset);
        for (unsigned int f = 0; f < srcMesh->mNumFaces; f++)
        {
            assert(srcMesh->mFaces[f].mNumIndices == 3);

            *dstIndex++ = srcMesh->mFaces[f].mIndices[0];
            *dstIndex++ = srcMesh->mFaces[f].mIndices[1];
            *dstIndex++ = srcMesh->mFaces[f].mIndices[2];

            *dstIndexDepth++ = srcMesh->mFaces[f].mIndices[0];
            *dstIndexDepth++ = srcMesh->mFaces[f].mIndices[1];
            *dstIndexDepth++ = srcMesh->mFaces[f].mIndices[2];
        }
    }

    ComputeAllBoundingBoxes();

    return true;
}
