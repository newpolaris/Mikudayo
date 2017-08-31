#include "stdafx.h"

#include "Physics.h"

#include "BulletDebugDraw.h"
#include "PrimitiveBatch.h"
#include "TextUtility.h"

namespace Physics
{
    BoolVar s_bInterpolation( "Application/Physics/Motion Interpolation", true );
    BoolVar s_bDebugDraw( "Application/Physics/Debug Draw", false );

    const float EarthGravity = 9.8f;
    SolverType m_SolverType = SOLVER_TYPE_SEQUENTIAL_IMPULSE;
    int m_SolverMode = SOLVER_SIMD |
        SOLVER_USE_WARMSTARTING |
        // SOLVER_RANDMIZE_ORDER |
        // SOLVER_INTERLEAVE_CONTACT_AND_FRICTION_CONSTRAINTS |
        // SOLVER_USE_2_FRICTION_DIRECTIONS |
        0;

	btSoftRigidDynamicsWorld* g_DynamicsWorld = nullptr;

    std::unique_ptr<btDefaultCollisionConfiguration> Config;
    std::unique_ptr<btBroadphaseInterface> Broadphase;
    std::unique_ptr<btCollisionDispatcher> Dispatcher;
    std::unique_ptr<btConstraintSolver> Solver;
    std::unique_ptr<btSoftRigidDynamicsWorld> DynamicsWorld;
    std::unique_ptr<BulletDebug::DebugDraw> DebugDrawer;
    btSoftBodyWorldInfo SoftBodyWorldInfo;
    btSoftBodyWorldInfo* g_SoftBodyWorldInfo = &SoftBodyWorldInfo;
    btConstraintSolver* CreateSolverByType( SolverType t );
};

using namespace Physics;

void EnterProfileZoneDefault(const char* name)
{
    PushProfilingMarker( Utility::MakeWStr(std::string(name)), nullptr );
}

void LeaveProfileZoneDefault()
{
    PopProfilingMarker( nullptr );
}

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
    PrimitiveBatch::Initialize();

    btSetCustomEnterProfileZoneFunc(EnterProfileZoneDefault);
    btSetCustomLeaveProfileZoneFunc(LeaveProfileZoneDefault);

    Config = std::make_unique<btSoftBodyRigidBodyCollisionConfiguration>();
    Broadphase = std::make_unique<btDbvtBroadphase>();
    Dispatcher = std::make_unique<btCollisionDispatcher>( Config.get() );
    Solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    Solver.reset( CreateSolverByType( m_SolverType ) );
    DynamicsWorld = std::make_unique<btSoftRigidDynamicsWorld>( Dispatcher.get(), Broadphase.get(), Solver.get(), Config.get() );
    SoftBodyWorldInfo.m_broadphase = Broadphase.get();
    SoftBodyWorldInfo.m_dispatcher = Dispatcher.get();
    SoftBodyWorldInfo.m_gravity = DynamicsWorld->getGravity();
    SoftBodyWorldInfo.m_sparsesdf.Initialize();

    ASSERT( DynamicsWorld != nullptr );
    DynamicsWorld->setGravity( btVector3( 0, -EarthGravity, 0 ) );
    DynamicsWorld->getSolverInfo().m_solverMode = m_SolverMode;

    DebugDrawer = std::make_unique<BulletDebug::DebugDraw>();
    DebugDrawer->setDebugMode(
        // btIDebugDraw::DBG_DrawAabb |
        // btIDebugDraw::DBG_DrawConstraints |
        // btIDebugDraw::DBG_DrawConstraintLimits |
        btIDebugDraw::DBG_DrawWireframe
    );
    DynamicsWorld->setDebugDrawer( DebugDrawer.get() );

    g_DynamicsWorld = DynamicsWorld.get();
}

void Physics::Shutdown( void )
{
    SoftBodyWorldInfo.m_sparsesdf.Reset();
    PrimitiveBatch::Shutdown();
    BulletDebug::Shutdown();

    int Len = (int)DynamicsWorld->getSoftBodyArray().size();
    for (int i = Len -1; i >= 0; i--)
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

void Physics::Render( GraphicsContext& Context, const Math::Matrix4& WorldToClip )
{
    ASSERT( DynamicsWorld.get() != nullptr );
    PrimitiveBatch::Flush( Context, WorldToClip );
    if (s_bDebugDraw)
    {
        DynamicsWorld->debugDrawWorld();
        for (int i = 0; i < DynamicsWorld->getSoftBodyArray().size(); i++)
		{
            btSoftBody*	psb = DynamicsWorld->getSoftBodyArray()[i];
            btSoftBodyHelpers::DrawFrame( psb, DynamicsWorld->getDebugDrawer() );
            btSoftBodyHelpers::Draw( psb, DynamicsWorld->getDebugDrawer(), DynamicsWorld->getDrawFlags() );
		}
        DebugDrawer->flush( Context, WorldToClip );
    }
}