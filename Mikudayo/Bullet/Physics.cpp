#include "stdafx.h"

#include "Physics.h"
#include "LinearMath.h"
#include "BulletDebugDraw.h"
#include "PrimitiveBatch.h"
#include "TextUtility.h"

namespace Physics
{
    BoolVar s_bInterpolation( "Application/Physics/Motion Interpolation", true );
    BoolVar s_bDebugDraw( "Application/Physics/Debug Draw", false );

    // use scalar from MMD-Agent
    // set default gravity 
    // some tweak for the simulation to match that of MikuMikuDance
    const float gScale = 10.f;
    const float EarthGravity = -9.8f * gScale;
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

    class BulletPicking
    {
    public:

        void PickingPreTickCallback( btDynamicsWorld* world, btScalar timeStep );
        bool PickBody( const btVector3& From, const btVector3& To, const btVector3& Forward );
        bool MovePickBody( const btVector3& From, const btVector3& To, const btVector3& Forward );
        void ReleasePickBody();

    protected:

        btScalar m_PickingDist;
        btVector3 m_PickingPos;
        btVector3 m_CameraPos;
        btVector3 m_Goal;
        btVector3 m_Impact;
        btVector3 m_CameraFoward;
        btRigidBody* m_PickedBody = nullptr;
        btSoftBody::Node* m_Node = nullptr;
        btPoint2PointConstraint* m_PickedConstraint = nullptr;
    } m_Picking;
};

using namespace Physics;

void BulletPicking::PickingPreTickCallback( btDynamicsWorld*, btScalar timeStep )
{
    //
    // Used bullet's Softbody example code
    //
    if (m_Node)
    {
        const btVector3	rayDir = (m_PickingPos - m_CameraPos).normalized();
        const btScalar O = btDot( m_Impact, m_CameraFoward );
        const btScalar den = btDot( m_CameraFoward, rayDir );
        if ((den*den) > 0)
        {
            const btScalar num = O - btDot( m_CameraFoward, m_CameraPos );
            const btScalar hit = num / den;
            if ((hit > 0) && (hit < 1500))
                m_Goal = m_CameraPos + rayDir*hit;
        }
        btVector3 delta = m_Goal - m_Node->m_x;
        static const btScalar maxdrag = 10, factor = 8;
        if (delta.length2() > maxdrag*maxdrag)
            delta = delta.normalized()*maxdrag;
        m_Node->m_v += delta * factor / timeStep;
    }
}

bool BulletPicking::PickBody( const btVector3 & From, const btVector3 & To, const btVector3 & Forward )
{
    m_CameraFoward = Forward;

    ReleasePickBody();
    btCollisionWorld::ClosestRayResultCallback rayCallback( From, To );
    DynamicsWorld->rayTest( From, To, rayCallback );
    if (!rayCallback.hasHit())
        return false;

    btVector3 pickPos = rayCallback.m_hitPointWorld;
    const btRigidBody* rb = btRigidBody::upcast( rayCallback.m_collisionObject );
    if (rb && !(rb->isStaticObject() || rb->isKinematicObject()))
    {
        m_PickedBody = const_cast<btRigidBody*>(rb);
        m_PickedBody->setActivationState( DISABLE_DEACTIVATION );
        btVector3 localPivot = rb->getCenterOfMassTransform().inverse() * pickPos;
        btPoint2PointConstraint* p2p = new btPoint2PointConstraint( *m_PickedBody, localPivot );
        DynamicsWorld->addConstraint( p2p, true );
        m_PickedConstraint = p2p;
        btScalar mousePickClamping = 30.f;
        p2p->m_setting.m_impulseClamp = mousePickClamping;
        //very weak constraint for picking
        p2p->m_setting.m_tau = 0.001f;
        m_Impact = pickPos;
    }
    const btSoftBody* csb = btSoftBody::upcast( rayCallback.m_collisionObject );
    if (csb)
    {
        btSoftBody* sb = const_cast<btSoftBody*>(csb);
        btSoftBody::sRayCast raycast;
        if (!sb->rayTest( From, To, raycast ))
            return false;
        m_Node = nullptr;
        m_Impact = From + (To - From)*raycast.fraction;
        if (raycast.feature == btSoftBody::eFeature::Tetra)
        {
            btSoftBody::Tetra& tet = raycast.body->m_tetras[raycast.index];
            m_Node = tet.m_n[0];
            for (int i = 1; i < 4; ++i)
                if ((m_Node->m_x - m_Impact).length2() >( tet.m_n[i]->m_x - m_Impact ).length2())
                    m_Node = tet.m_n[i];
        }
        else if (raycast.feature == btSoftBody::eFeature::Face)
        {
            btSoftBody::Face& f = raycast.body->m_faces[raycast.index];
            m_Node = f.m_n[0];
            for (int i = 1; i < 3; ++i)
                if ((m_Node->m_x - m_Impact).length2() >( f.m_n[i]->m_x - m_Impact ).length2())
                    m_Node = f.m_n[i];
        }
        if (m_Node)
            m_Goal = m_Node->m_x;
    }
    m_CameraPos = From;
    m_PickingPos = To;
    m_PickingDist = (pickPos - From).length();

    return true;
}

bool BulletPicking::MovePickBody( const btVector3 & From, const btVector3 & To, const btVector3 & Forward )
{
    m_CameraFoward = Forward;
    if (m_PickedBody && m_PickedConstraint)
    {
        btPoint2PointConstraint* pickCon = static_cast<btPoint2PointConstraint*>(m_PickedConstraint);
        if (pickCon)
        {
            //keep it at the same picking distance
            btVector3 dir = To - From;
            dir.normalize();
            dir *= m_PickingDist;

            btVector3 newPivotB = From + dir;
            pickCon->setPivotB( newPivotB );
        }
    }
    m_CameraPos = From;
    m_PickingPos = To;
    return true;
}

void BulletPicking::ReleasePickBody()
{
    if (m_PickedBody)
    {
        m_PickedBody->forceActivationState( ACTIVE_TAG );
        m_PickedBody->setDeactivationTime( 0.f );
    }
    if (m_PickedConstraint)
    {
        DynamicsWorld->removeConstraint( m_PickedConstraint );
        delete m_PickedConstraint;
        m_PickedConstraint = nullptr;
    }
    m_Node = nullptr;
    m_PickedBody = nullptr;
    m_PickedConstraint = nullptr;
}

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
    DynamicsWorld->setGravity( btVector3( 0, EarthGravity, 0 ) );
    DynamicsWorld->getSolverInfo().m_solverMode = m_SolverMode;

    DebugDrawer = std::make_unique<BulletDebug::DebugDraw>();
    DebugDrawer->setDebugMode(
        // btIDebugDraw::DBG_DrawAabb |
        btIDebugDraw::DBG_DrawConstraints |
        btIDebugDraw::DBG_DrawConstraintLimits
        // btIDebugDraw::DBG_DrawWireframe
    );
    DynamicsWorld->setDebugDrawer( DebugDrawer.get() );
    auto pickCallback = []( btDynamicsWorld * world, btScalar timeStep ) {
        m_Picking.PickingPreTickCallback( world, timeStep );
    };
    DynamicsWorld->setInternalTickCallback( pickCallback, nullptr, true );
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
    ScopedTimer _prof( L"Physics" );

    DynamicsWorld->setLatencyMotionStateInterpolation( s_bInterpolation );
    ASSERT(DynamicsWorld.get() != nullptr);
    DynamicsWorld->stepSimulation( deltaT, 1 );
}

void Physics::Render( GraphicsContext& Context, const Matrix4& WorldToClip )
{
    PrimitiveBatch::Flush( Context, WorldToClip );
}

void Physics::RenderDebug( GraphicsContext& Context, const Matrix4& WorldToClip )
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
        DebugDrawer->flush( Context, WorldToClip );
    }
}

bool Physics::PickBody( const btVector3& From, const btVector3& To, const btVector3& Forward )
{
    return m_Picking.PickBody( From, To, Forward );
}

bool Physics::MovePickBody( const btVector3& From, const btVector3& To, const btVector3& Forward )
{
    return m_Picking.MovePickBody( From, To, Forward );
}

void Physics::ReleasePickBody()
{
    m_Picking.ReleasePickBody();
}
