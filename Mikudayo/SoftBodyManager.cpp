#include "stdafx.h"
#include "SoftBodyManager.h"
#include "Bullet/Physics.h"

btSoftBody* SoftBodyManager::CreateSoftBody( const SoftBodyGeometry& Geometry )
{
    return nullptr;
}

SoftBodyManager::SoftBodyManager()
{
}

SoftBodyManager::~SoftBodyManager()
{
}

btSoftBody* SoftBodyManager::LoadFromGeometry( const SoftBodyGeometry& Geometry )
{
    return nullptr;
}

btSoftBody* SoftBodyManager::LoadFromFile( const std::string& Filename )
{
    if (!LoadGeometryFromFile( Filename ))
        return nullptr;
    return LoadFromGeometry( m_Geometry[Filename] );
}

bool SoftBodyManager::LoadGeometryFromFile( const std::string& Filename )
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

    const aiScene* scene = importer.ReadFile(Filename,
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate |
        aiProcess_RemoveComponent |
        aiProcess_GenSmoothNormals |
        aiProcess_SplitLargeMeshes |
        aiProcess_ValidateDataStructure |
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_SortByPType |
        aiProcess_FindInvalidData |
        aiProcess_GenUVCoords |
        aiProcess_TransformUVCoords |
        aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph);

    if (scene == nullptr)
        return false;

    unsigned int meshCount = scene->mNumMeshes;
    unsigned int vertexCount = 0;
    unsigned int indexCount = 0;

    // first pass, count everything
    for (unsigned int meshIndex = 0; meshIndex < meshCount; meshIndex++)
    {
        const aiMesh *srcMesh = scene->mMeshes[meshIndex];
        assert(srcMesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);
        // color rendering
        vertexCount += srcMesh->mNumVertices;
        indexCount += srcMesh->mNumFaces * 3;
    }
    // allocate storage
    std::vector<XMFLOAT3> VertexStore(vertexCount);
    std::vector<uint32_t> IndexStore(indexCount);

    unsigned char* pVertexData = (unsigned char*)VertexStore.data();
    unsigned char* pIndexData = (unsigned char*)IndexStore.data();

    // second pass, fill in vertex and index data
    float *dstPos = (float*)(pVertexData);
    uint32_t *dstIndex = (uint32_t*)(pIndexData);
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
    {
        const aiMesh *srcMesh = scene->mMeshes[meshIndex];
        for (unsigned int v = 0; v < srcMesh->mNumVertices; v++)
        {
            if (srcMesh->mVertices)
            {
                dstPos[0] = srcMesh->mVertices[v].x;
                dstPos[1] = srcMesh->mVertices[v].y;
                dstPos[2] = srcMesh->mVertices[v].z;
            }
            dstPos += 3;
        }
        for (unsigned int f = 0; f < srcMesh->mNumFaces; f++)
        {
            assert(srcMesh->mFaces[f].mNumIndices == 3);

            *dstIndex++ = srcMesh->mFaces[f].mIndices[0];
            *dstIndex++ = srcMesh->mFaces[f].mIndices[1];
            *dstIndex++ = srcMesh->mFaces[f].mIndices[2];
        }
    }
    m_Geometry[Filename] = SoftBodyGeometry { VertexStore, IndexStore };

    return true;
}
