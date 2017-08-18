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
#include "PhysicsPrimitive.h"
#include "PrimitiveBatch.h"

#include "GameCore.h"
#include "EngineTuning.h"
#include "Utility.h"
#define BT_THREADSAFE 1
#define BT_NO_SIMD_OPERATOR_OVERLOADS 1
#include "btBulletDynamicsCommon.h"
#include "BaseRigidBody.h"
#include "BulletDebugDraw.h"
#include "MultiThread.inl"
#include "LinearMath/btThreads.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"

using namespace Math;

namespace Physics
{
    BoolVar s_bInterpolation( "Application/Physics/Motion Interpolation", true );
    BoolVar s_bDebugDraw( "Application/Physics/Debug Draw", true );

    // bullet needs to define BT_THREADSAFE and (BT_USE_OPENMP || BT_USE_PPL || BT_USE_TBB)
    const bool bMultithreadCapable = false;
    const float EarthGravity = 9.8f;
    SolverType m_SolverType = SOLVER_TYPE_SEQUENTIAL_IMPULSE;
    int m_SolverMode = SOLVER_SIMD |
        SOLVER_USE_WARMSTARTING |
        // SOLVER_RANDMIZE_ORDER |
        // SOLVER_INTERLEAVE_CONTACT_AND_FRICTION_CONSTRAINTS |
        // SOLVER_USE_2_FRICTION_DIRECTIONS |
        0;

	btDynamicsWorld* g_DynamicsWorld = nullptr;

    std::unique_ptr<btDefaultCollisionConfiguration> Config;
    std::unique_ptr<btBroadphaseInterface> Broadphase;
    std::unique_ptr<btCollisionDispatcher> Dispatcher;
    std::unique_ptr<btConstraintSolver> Solver;
    std::unique_ptr<btSoftRigidDynamicsWorld> DynamicsWorld;
    std::unique_ptr<BulletDebug::DebugDraw> DebugDrawer;
    btSoftBodyWorldInfo SoftBodyWorldInfo;

    btConstraintSolver* CreateSolverByType( SolverType t );
};

btConstraintSolver* Physics::CreateSolverByType( SolverType t )
{
    btMLCPSolverInterface* mlcpSolver = NULL;
    switch (t)
    {
    case SOLVER_TYPE_SEQUENTIAL_IMPULSE:
        return new btSequentialImpulseConstraintSolver();
    case SOLVER_TYPE_NNCG:
        return new btNNCGConstraintSolver();
    case SOLVER_TYPE_MLCP_PGS:
        mlcpSolver = new btSolveProjectedGaussSeidel();
        break;
    case SOLVER_TYPE_MLCP_DANTZIG:
        mlcpSolver = new btDantzigSolver();
        break;
    case SOLVER_TYPE_MLCP_LEMKE:
        mlcpSolver = new btLemkeSolver();
        break;
    default: {}
    }
    if (mlcpSolver)
        return new btMLCPSolver( mlcpSolver );
    return NULL;
}

void Physics::Initialize( void )
{
    BulletDebug::Initialize();
    gTaskMgr.init(8);

    if (bMultithreadCapable)
    {
        btDefaultCollisionConstructionInfo cci;
        cci.m_defaultMaxPersistentManifoldPoolSize = 80000;
        cci.m_defaultMaxCollisionAlgorithmPoolSize = 80000;
        Config = std::make_unique<btSoftBodyRigidBodyCollisionConfiguration >( cci );

#if USE_PARALLEL_NARROWPHASE
        Dispatcher = std::make_unique<MyCollisionDispatcher>( Config.get() );
#else
        Dispatcher = std::make_unique<btCollisionDispatcher>( Config.get() );
#endif //USE_PARALLEL_NARROWPHASE

        Broadphase = std::make_unique<btDbvtBroadphase>();

#if USE_PARALLEL_ISLAND_SOLVER
        {
            btConstraintSolver* solvers[ BT_MAX_THREAD_COUNT ];
            int maxThreadCount = btMin( int(BT_MAX_THREAD_COUNT), TaskManager::getMaxNumThreads() );
            for ( int i = 0; i < maxThreadCount; ++i )
                solvers[ i ] = CreateSolverByType( m_SolverType );
            Solver.reset( new MyConstraintSolverPool( solvers, maxThreadCount ) );
        }
#else
        Solver.reset( CreateSolverByType( m_SolverType ) );
#endif //#if USE_PARALLEL_ISLAND_SOLVER

        DynamicsWorld = std::make_unique<btSoftRigidDynamicsWorld>( Dispatcher.get(), Broadphase.get(), Solver.get(), Config.get() );

#if USE_PARALLEL_ISLAND_SOLVER
        if ( btSimulationIslandManagerMt* islandMgr = dynamic_cast<btSimulationIslandManagerMt*>( DynamicsWorld->getSimulationIslandManager() ) )
            islandMgr->setIslandDispatchFunction( parallelIslandDispatch );
#endif //#if USE_PARALLEL_ISLAND_SOLVER
    }
    else
    {
        Config = std::make_unique<btSoftBodyRigidBodyCollisionConfiguration>();
        Broadphase = std::make_unique<btDbvtBroadphase>();
        Dispatcher = std::make_unique<btCollisionDispatcher>( Config.get() );
        Solver = std::make_unique<btSequentialImpulseConstraintSolver>();
        Solver.reset( CreateSolverByType( m_SolverType ) );
        DynamicsWorld = std::make_unique<btSoftRigidDynamicsWorld>( Dispatcher.get(), Broadphase.get(), Solver.get(), Config.get() );
    }
    SoftBodyWorldInfo.m_broadphase = Broadphase.get();
    SoftBodyWorldInfo.m_dispatcher = Dispatcher.get();
    SoftBodyWorldInfo.m_gravity = DynamicsWorld->getGravity();
    SoftBodyWorldInfo.m_sparsesdf.Initialize();

    ASSERT( DynamicsWorld != nullptr );
    DynamicsWorld->setGravity( btVector3( 0, -EarthGravity, 0 ) );
    DynamicsWorld->setInternalTickCallback( profileBeginCallback, NULL, true );
    DynamicsWorld->setInternalTickCallback( profileEndCallback, NULL, false );
    DynamicsWorld->getSolverInfo().m_solverMode = m_SolverMode;

    DebugDrawer = std::make_unique<BulletDebug::DebugDraw>();
    DebugDrawer->setDebugMode(
        btIDebugDraw::DBG_DrawConstraints |
        btIDebugDraw::DBG_DrawConstraintLimits |
        btIDebugDraw::DBG_DrawAabb |
        btIDebugDraw::DBG_DrawWireframe );
    DynamicsWorld->setDebugDrawer( DebugDrawer.get() );

    g_DynamicsWorld = DynamicsWorld.get();
}

void Physics::Shutdown( void )
{
    gTaskMgr.shutdown();
    SoftBodyWorldInfo.m_sparsesdf.Reset();
    BulletDebug::Shutdown();
    for (int i = 0; i < DynamicsWorld->getSoftBodyArray().size(); i++)
    {
        btSoftBody*	psb = DynamicsWorld->getSoftBodyArray()[i];
        DynamicsWorld->removeSoftBody( psb );
        delete psb;
    }
    ASSERT(DynamicsWorld->getNumCollisionObjects() == 0,
        "Remove all rigidbody objects from world");

    DynamicsWorld.reset( nullptr );
    g_DynamicsWorld = nullptr;
}

void Physics::Update( float deltaT )
{
    DynamicsWorld->setLatencyMotionStateInterpolation( s_bInterpolation );
    ASSERT(DynamicsWorld.get() != nullptr);
    DynamicsWorld->stepSimulation( deltaT, 1 );
}

void Physics::Render( GraphicsContext& Context, const Matrix4& ClipToWorld )
{
    ASSERT( DynamicsWorld.get() != nullptr );
    if (s_bDebugDraw)
    {
        DynamicsWorld->debugDrawWorld();
        for (int i = 0; i < DynamicsWorld->getSoftBodyArray().size(); i++)
		{
            btSoftBody*	psb = DynamicsWorld->getSoftBodyArray()[i];
            btSoftBodyHelpers::DrawFrame( psb, DynamicsWorld->getDebugDrawer() );
            btSoftBodyHelpers::Draw( psb, DynamicsWorld->getDebugDrawer(), DynamicsWorld->getDrawFlags() );
		}
        DebugDrawer->flush( Context, ClipToWorld );
    }
}

void Physics::Profile( ProfileStatus& Status )
{
    Status.NumIslands = gNumIslands;
    Status.NumCollisionObjects = DynamicsWorld->getNumCollisionObjects();
    int numContacts = 0;
    int numManifolds = Dispatcher->getNumManifolds();
    for (int i = 0; i < numManifolds; ++i)
    {
        const btPersistentManifold* man = Dispatcher->getManifoldByIndexInternal( i );
        numContacts += man->getNumContacts();
    }
    Status.NumManifolds = numManifolds;
    Status.NumContacts = numContacts;
    Status.NumThread = gTaskMgr.getNumThreads();
    Status.InternalTimeStep = gProfiler.getAverageTime( Profiler::kRecordInternalTimeStep )*0.001f;
    if (bMultithreadCapable)
    {
        Status.DispatchAllCollisionPairs = gProfiler.getAverageTime( Profiler::kRecordDispatchAllCollisionPairs )*0.001f;
        Status.DispatchIslands = gProfiler.getAverageTime( Profiler::kRecordDispatchIslands )*0.001f;
        Status.PredictUnconstrainedMotion = gProfiler.getAverageTime( Profiler::kRecordPredictUnconstrainedMotion )*0.001f;
        Status.CreatePredictiveContacts = gProfiler.getAverageTime( Profiler::kRecordCreatePredictiveContacts )*0.001f;
        Status.IntegrateTransforms = gProfiler.getAverageTime( Profiler::kRecordIntegrateTransforms )*0.001f;
    }
}

namespace {
    std::vector<Primitive::PhysicsPrimitivePtr> m_Models;
};

void CreateSoftBody(const btScalar s,
    const int numX,
    const int numY,
    const int fixed = 1+2 )
{
    btSoftBody* cloth = btSoftBodyHelpers::CreatePatch(
        Physics::SoftBodyWorldInfo,
        btVector3( -s / 2, s + 1, 0 ),
        btVector3( +s / 2, s + 1, 0 ),
        btVector3( -s / 2, s + 1, +s ),
        btVector3( +s / 2, s + 1, +s ),
        numX, numY,
        fixed, true );

	cloth->m_cfg.piterations = 5;
	cloth->getCollisionShape()->setMargin(0.001f);
	cloth->generateBendingConstraints(2,cloth->appendMaterial());
	cloth->setTotalMass(10);
	cloth->m_cfg.citerations = 10;
    cloth->m_cfg.diterations = 10;
	cloth->m_cfg.kDP = 0.005f;
	Physics::DynamicsWorld->addSoftBody(cloth);
}

using namespace GameCore;
using namespace Graphics;
using namespace Math;

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

    Camera m_Camera;
    std::auto_ptr<CameraController> m_CameraController;

    Matrix4 m_ViewMatrix;
    Matrix4 m_ProjMatrix;
    Matrix4 m_ViewProjMatrix;
    D3D11_VIEWPORT m_MainViewport;
    D3D11_RECT m_MainScissor;

    GraphicsPSO m_DepthPSO;
    GraphicsPSO m_CutoutDepthPSO;
    GraphicsPSO m_ModelPSO;
};

CREATE_APPLICATION( SoftbodyExample )

void SoftbodyExample::Startup( void )
{
    TextureManager::Initialize( L"Textures" );
    Physics::Initialize();
    PrimitiveBatch::Initialize();

    const Vector3 eye = Vector3(0.0f, 10.0f, 10.0f);
    m_Camera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
    m_CameraController.reset(new CameraController(m_Camera, Vector3(kYUnitVector)));

    m_Models.push_back( std::move( Primitive::CreatePhysicsPrimitive(
        { Physics::kPlaneShape, 0.f, Vector3( kZero ), Vector3( kZero ) }
    ) ) );

    {
        const btScalar s = 4; // size of cloth patch
        const int NUM_X = 31; // vertices on X axis
        const int NUM_Z = 31; // vertices on Z axis
        CreateSoftBody( s, NUM_X, NUM_Z );
    }
}

void SoftbodyExample::Cleanup( void )
{
    for (auto& model : m_Models)
        model->Destroy();
    m_Models.clear();

    PrimitiveBatch::Shutdown();
    Physics::Shutdown();
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
        ScopedTimer _prof( L"Primitive Draw", gfxContext );
        for (auto& model : m_Models)
            model->Draw(m_Camera.GetWorldSpaceFrustum());
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
