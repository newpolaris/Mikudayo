#include <memory>
#include <vector>

#include "BulletDebugDraw.h"
#define BT_NO_SIMD_OPERATOR_OVERLOADS 1
#include "btBulletDynamicsCommon.h"
#include "BaseRigidBody.h"
#include "PhysicsPrimitive.h"
#include "GameCore.h"
#include "EngineTuning.h"
#include "Utility.h"

using namespace Math;

namespace Physics
{
    BoolVar s_bInterpolation( "Application/Physics/Motion Interpolation", true );
    BoolVar s_bDebugDraw( "Application/Physics/Debug Draw", false );

	btDynamicsWorld* g_DynamicsWorld = nullptr;

    btDefaultCollisionConfiguration Config;
    std::unique_ptr<btBroadphaseInterface> Broadphase;
    std::unique_ptr<btCollisionDispatcher> Dispatcher;
    std::unique_ptr<btSequentialImpulseConstraintSolver> Solver;
    std::unique_ptr<btDiscreteDynamicsWorld> DynamicsWorld;
    std::unique_ptr<BulletDebug::DebugDraw> DebugDrawer;

    void Initialize( void );
    void Shutdown( void );
    void Update( float deltaT );
    void Render( GraphicsContext& Context, const Matrix4& ClipToWorld );
};

void Physics::Initialize( void )
{
    BulletDebug::Initialize();
    Primitive::Initialize();

    Broadphase = std::make_unique<btDbvtBroadphase>();
    Dispatcher = std::make_unique<btCollisionDispatcher>( &Config );
    Solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    const float EarthGravity = 9.8f;
    DynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>( Dispatcher.get(), Broadphase.get(), Solver.get(), &Config );
    ASSERT( DynamicsWorld != nullptr );
    DynamicsWorld->setGravity( btVector3( 0, -EarthGravity, 0 ) );

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
    Primitive::Shutdown();
    BulletDebug::Shutdown();

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
        DebugDrawer->flush( Context, ClipToWorld );
    }
}

