#include "stdafx.h"
#include "BaseMaterial.h"
#include "BaseMesh.h"
#include "ModelAccessory.h"

bool ModelAccessory::Load( const ModelInfo& info )
{
    BaseModel::Load( info );

    const std::string name = Utility::MakeStr( m_FileName );
    const char* filename = name.c_str();
    if (!AssimpModel::Load(filename))
        return false;

    for (uint32_t i = 0; i < m_Header.materialCount; i++)
    {
        MaterialPtr material = std::make_shared<BaseMaterial>();
        material->ambient = m_pMaterial[i].ambient;
        material->diffuse = m_pMaterial[i].diffuse;
        material->specular = m_pMaterial[i].specular;
        material->emissive = m_pMaterial[i].emissive;
        material->transparent = m_pMaterial[i].transparent;
        material->opacity = m_pMaterial[i].opacity;
        material->shininess = m_pMaterial[i].shininess;
        material->specularStrength = m_pMaterial[i].specularStrength;

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
