#include <iostream>

#include "GameCore.h"
#include "GraphicsCore.h"
#include "PipelineState.h"
#include "CommandContext.h"
#include "GpuBuffer.h"
#include "InputLayout.h"
#include "ColorBuffer.h"
#include "BufferManager.h"
#include "Camera.h"
#include "CameraController.h"
#include "SamplerManager.h"
#include "GameInput.h"
#include "Physics.h"
#include "TextureManager.h"
#include "ModelBase.h"
#include "ModelLoader.h"
#include "PhysicsPrimitive.h"
#include "PrimitiveBatch.h"
#include "Pmx.h"
#include "BulletSoftBody/btSoftBody.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"

#include "CompiledShaders/PmxOpaqueVS.h"
#include "CompiledShaders/PmxOpaquePS.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <boost/filesystem/path.hpp>

using namespace Math;

using namespace GameCore;
using namespace Graphics;
using namespace Math;

namespace Pmx {
	std::vector<InputDesc> InputDescriptor
	{
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_ID", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "EDGE_FLAT", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
}

class SoftbodyExample : public GameCore::IGameApp
{
public:
	SoftbodyExample()
	{
	}

	virtual void Startup( void ) override;
	virtual void Cleanup( void ) override;

	virtual void Update( float deltaT ) override;
	virtual void RenderScene( void ) override;
    virtual void RenderUI( GraphicsContext & Context ) override;

private:

    void RenderObjects( GraphicsContext& gfxContext, const Matrix4 & ViewMat, const Matrix4 & ProjMat, eObjectFilter Filter );
    void LoadObjectFile( const std::string& Name, const std::string& Filename );

    Camera m_Camera;
    std::auto_ptr<CameraController> m_CameraController;

    Matrix4 m_ViewMatrix;
    Matrix4 m_ProjMatrix;
    Matrix4 m_ViewProjMatrix;
    D3D11_VIEWPORT m_MainViewport;
    D3D11_RECT m_MainScissor;

    GraphicsPSO m_ModelPSO;
    GraphicsPSO m_BlendPSO;

    using BasicGeometry = std::pair<std::vector<btVector3>, std::vector<uint32_t>>;
    std::map<std::string, BasicGeometry> m_BasicGeometry;
    std::vector<std::shared_ptr<Graphics::IRenderObject>> m_Models;
    std::vector<Primitive::PhysicsPrimitivePtr> m_Primitive;
};

CREATE_APPLICATION( SoftbodyExample )

btSoftBody* GetSoftBody(btScalar* Vertices, int* Indices, size_t FaceCount)
{
	btSoftBody* psb = btSoftBodyHelpers::CreateFromTriMesh(
        *Physics::g_SoftBodyWorldInfo, Vertices, Indices, FaceCount );

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
    psb->m_cfg.kMT = 0.5;
    psb->m_cfg.kCHR = 1.0; // Rigid contacts hardness [0,1]
    psb->m_cfg.kKHR = 0.1; // Kinetic contacts hardness [0,1]
    psb->m_cfg.kSHR = 1.0; // Soft contacts hardness [0,1]
    psb->m_cfg.kAHR = 0.7; // Anchors hardness [0,1]
    psb->m_cfg.viterations = 0; // Velocities solver iterations
    psb->m_cfg.piterations = 1; // Positions solver iterations
    psb->m_cfg.diterations = 0; // Drift solver iterations
    psb->m_cfg.citerations = 1; // Cluster solver iterations
    psb->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;//SB同士のコリジョン
    psb->randomizeConstraints();
    psb->generateClusters( 3 );
    psb->scale( btVector3(0.5, 0.5, 0.5) );
    psb->setTotalMass( 10.f, true );
    psb->setPose( true, true );
    return psb;
}

void SoftbodyExample::Startup( void )
{
    TextureManager::Initialize( L"Textures" );
    ModelBase::Initialize();
    Physics::Initialize();
    PrimitiveBatch::Initialize();

    const Vector3 eye = Vector3(0.0f, 10.0f, 10.0f);
    m_Camera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
    m_CameraController.reset(new CameraController(m_Camera, Vector3(kYUnitVector)));

    std::vector<Primitive::PhysicsPrimitiveInfo> primitves = {
        { Physics::kPlaneShape, 0.f, Vector3( kZero ), Vector3( 0, 0, 0 ) },
        { Physics::kBoxShape, 20.f, Vector3( 19,1,19 ), Vector3( 0, 2, 0 ) },
    };

    for (auto& info : primitves)
        m_Primitive.push_back( std::move( Primitive::CreatePhysicsPrimitive( info ) ) );

	LoadObjectFile("mikudayo", "Models/mikudayo-LOD2.obj");
    auto& data = m_BasicGeometry["mikudayo"];
    for (auto k : { 10, 40, 80, 100 })
    {
        auto psb = GetSoftBody( (btScalar*)data.first.data(), (int*)data.second.data(), data.second.size() / 3 );
        psb->translate( btVector3( -.1, k, 0.1 ) );
        Physics::g_DynamicsWorld->addSoftBody( psb );
    }

    /*
    ModelLoader Loader( L"Models/mikudayo-3_6_.pmx", L"", XMFLOAT3( 15, 0, 0 ) );
    auto model = Loader.Load();
    if (model)
    {
        m_Models.push_back( model );
    }
    */

	m_ModelPSO.SetRasterizerState( RasterizerDefault );
	m_ModelPSO.SetBlendState( BlendDisable );
	m_ModelPSO.SetInputLayout( static_cast<UINT>(Pmx::InputDescriptor.size()), Pmx::InputDescriptor.data() );
    m_ModelPSO.SetDepthStencilState( DepthStateReadWrite );
    m_ModelPSO.SetVertexShader( MY_SHADER_ARGS( g_pPmxOpaqueVS ) );
    m_ModelPSO.SetPixelShader( MY_SHADER_ARGS( g_pPmxOpaquePS ) );
    m_ModelPSO.Finalize();

    m_BlendPSO = m_ModelPSO;
    m_BlendPSO.SetRasterizerState( RasterizerDefault );
    m_BlendPSO.SetBlendState( BlendTraditional );
    m_BlendPSO.Finalize();
}

void SoftbodyExample::Cleanup( void )
{
    m_ModelPSO.Destroy();
    m_BlendPSO.Destroy();

    for (auto& model : m_Primitive)
        model->Destroy();
    m_Primitive.clear();
    m_Models.clear();

    PrimitiveBatch::Shutdown();
    Physics::Shutdown();
    ModelBase::Shutdown();
}

void SoftbodyExample::Update( float deltaT )
{
    ScopedTimer _prof( L"Bullet Update" );

    if (!EngineProfiling::IsPaused())
    {
        Physics::Update( deltaT );
    }

    m_CameraController->Update( deltaT );
    m_ViewMatrix = m_Camera.GetViewMatrix();
    m_ProjMatrix = m_Camera.GetProjMatrix();
    m_ViewProjMatrix = m_Camera.GetViewProjMatrix();

	m_MainViewport.Width = (float)g_SceneColorBuffer.GetWidth();
	m_MainViewport.Height = (float)g_SceneColorBuffer.GetHeight();
	m_MainViewport.MinDepth = 0.0f;
	m_MainViewport.MaxDepth = 1.0f;

	m_MainScissor.left = 0;
	m_MainScissor.top = 0;
	m_MainScissor.right = (LONG)g_SceneColorBuffer.GetWidth();
	m_MainScissor.bottom = (LONG)g_SceneColorBuffer.GetHeight();
}

void SoftbodyExample::RenderObjects( GraphicsContext& gfxContext, const Matrix4& ViewMat, const Matrix4& ProjMat, eObjectFilter Filter )
{
    struct VSConstants
    {
        Matrix4 view;
        Matrix4 projection;
    } vsConstants;
    vsConstants.view = ViewMat;
    vsConstants.projection = ProjMat;
	gfxContext.SetDynamicConstantBufferView( 0, sizeof(vsConstants), &vsConstants, { kBindVertex } );
    for (auto& model : m_Models)
        model->Draw( gfxContext, Filter );
}

void SoftbodyExample::RenderScene( void )
{
    struct {
        Matrix4 ViewToClip;
    } vsConstants { m_ProjMatrix*m_ViewMatrix };
    struct {
        Vector3 CameraPosition;
    } psConstants { m_Camera.GetPosition() };

    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");

    gfxContext.SetViewportAndScissor( m_MainViewport, m_MainScissor );
    gfxContext.ClearColor( g_SceneColorBuffer );
    gfxContext.ClearDepth( g_SceneDepthBuffer );
    gfxContext.SetRenderTarget( g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV() );
    gfxContext.SetDynamicSampler( 0, SamplerLinearWrap, { kBindPixel } );
    gfxContext.SetDynamicConstantBufferView( 0, sizeof(vsConstants), &vsConstants, { kBindVertex } );
    gfxContext.SetDynamicConstantBufferView( 0, sizeof(psConstants), &psConstants, { kBindPixel } );
    {
        gfxContext.SetPipelineState( m_ModelPSO );
        RenderObjects( gfxContext, m_ViewMatrix, m_ProjMatrix, kOpaque );
        RenderObjects( gfxContext, m_ViewMatrix, m_ProjMatrix, kOverlay );
        ModelBase::Flush( gfxContext );
        gfxContext.SetPipelineState( m_BlendPSO );
        RenderObjects( gfxContext, m_ViewMatrix, m_ProjMatrix, kTransparent );
        PrimitiveBatch::Flush( gfxContext );
    }
    gfxContext.Finish();
}

void SoftbodyExample::RenderUI( GraphicsContext& Context )
{
    Physics::Render( Context, m_ViewProjMatrix );
	int32_t x = g_OverlayBuffer.GetWidth() - 500;

    Physics::ProfileStatus Status;
    Physics::Profile( Status );

	TextContext UiContext(Context);
	UiContext.Begin();
    UiContext.ResetCursor( float(x), 10.f );
    UiContext.SetColor(Color( 0.7f, 1.0f, 0.7f ));

#define DebugAttribute(Attribute) \
    UiContext.DrawFormattedString( ""#Attribute " %3d", Status.Attribute ); \
    UiContext.NewLine();

    DebugAttribute(NumIslands);
    DebugAttribute(NumCollisionObjects);
    DebugAttribute(NumManifolds);
    DebugAttribute(NumContacts);
    DebugAttribute(NumThread);
#undef DebugAttribute

#define DebugAttribute(Attribute) \
    UiContext.DrawFormattedString( ""#Attribute " %5.3f", Status.Attribute ); \
    UiContext.NewLine();

    DebugAttribute(InternalTimeStep);
    DebugAttribute(DispatchAllCollisionPairs);
    DebugAttribute(DispatchIslands);
    DebugAttribute(PredictUnconstrainedMotion);
    DebugAttribute(CreatePredictiveContacts);
    DebugAttribute(IntegrateTransforms);
#undef DebugAttribute
}

void SoftbodyExample::LoadObjectFile(const std::string& Name, const std::string& Filename)
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

    const aiScene *scene = importer.ReadFile(Filename,
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

    unsigned int meshCount = scene->mNumMeshes;
    unsigned int vertexCount = 0;
    unsigned int indexCount = 0;

    // first pass, count everything
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
    {
        const aiMesh *srcMesh = scene->mMeshes[meshIndex];
        assert(srcMesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);
        // color rendering
        vertexCount += srcMesh->mNumVertices;
        indexCount += srcMesh->mNumFaces * 3;
    }
    // allocate storage
    std::vector<btVector3> VertexStore(vertexCount);
    std::vector<uint32_t> IndexStore(indexCount);

    unsigned int vertexStride = sizeof(float) * 3;
    unsigned char* pVertexData = (unsigned char*)VertexStore.data();
    unsigned int indexStride = sizeof(uint32_t);
    unsigned char* pIndexData = (unsigned char*) IndexStore.data();

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
    m_BasicGeometry.insert({Name, { VertexStore, IndexStore}});
}