#include "stdafx.h"
#include "SoftBodyManager.h"
#include "Bullet/Physics.h"

btSoftBody* SoftBodyManager::CreateSoftBody( const SoftBodyGeometry& Geometry )
{
    btSoftBody* psb = btSoftBodyHelpers::CreateFromTriMesh(
        *Physics::g_SoftBodyWorldInfo,
        reinterpret_cast<const btScalar*>(Geometry.Positions.data()),
        reinterpret_cast<const int*>(Geometry.Indices.data()),
        static_cast<int>(Geometry.Indices.size() / 3) );

    btSoftBody::Material* pm = psb->appendMaterial();
    pm->m_kLST = 1.0;
    pm->m_kAST = 1.0;
    pm->m_kVST = 1.0;
    pm->m_flags -= btSoftBody::fMaterial::DebugDraw;

    psb->generateBendingConstraints( 2, pm );
    psb->m_cfg.kVCF = 1.0; // Velocities correction factor (Baumgarte)
    psb->m_cfg.kDP = 0.0; // Damping coefficient [0,1]
    psb->m_cfg.kDG = 0.0; // Drag coefficient [0,+inf]
    psb->m_cfg.kLF = 0.0; // Lift coefficient [0,+inf]
    psb->m_cfg.kPR = 0.5;
    psb->m_cfg.kVC = 0.001;
    psb->m_cfg.kDF = 0.5;
    psb->m_cfg.kMT = 0.1;
    psb->m_cfg.kCHR = 0.1; // Rigid contacts hardness [0,1]
    psb->m_cfg.kKHR = 0.1; // Kinetic contacts hardness [0,1]
    psb->m_cfg.kSHR = 0.1; // Soft contacts hardness [0,1]
    psb->m_cfg.kAHR = 0.1; // Anchors hardness [0,1]
    psb->m_cfg.viterations = 0; // Velocities solver iterations
    psb->m_cfg.piterations = 2; // Positions solver iterations
    psb->m_cfg.diterations = 0; // Drift solver iterations
    psb->m_cfg.citerations = 1; // Cluster solver iterations
    psb->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;
    psb->randomizeConstraints();
    psb->generateClusters( 3 );
    psb->scale( btVector3( 0.5, 0.5, 0.5 ) );
    psb->setTotalMass( 10.f, true );
    psb->setPose( true, true );

    return psb;
}

SoftBodyManager::SoftBodyManager()
{
}

SoftBodyManager::~SoftBodyManager()
{
}

btSoftBody* SoftBodyManager::LoadFromGeometry( const SoftBodyGeometry& Geometry )
{
    btSoftBody* psb = CreateSoftBody( Geometry );
    Physics::g_DynamicsWorld->addSoftBody( psb );
    return psb;
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
