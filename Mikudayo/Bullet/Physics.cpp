﻿#include "stdafx.h"

#include "Physics.h"
#include "LinearMath.h"
#include "BulletDebugDraw.h"
#include "PrimitiveBatch.h"
#include "TextUtility.h"

//
// TODO:
// Teleport support : http://www.bulletphysics.org/Bullet/phpBB3/viewtopic.php?f=9&t=1150
// Scaling support : https://gamedev.stackexchange.com/questions/90767/bullet-physics-scaling
//
namespace Physics
{
    BoolVar m_bInterpolation( "Application/Physics/Motion Interpolation", true );
    BoolVar s_bDebugDraw( "Application/Physics/Debug Draw", false );

    // use scalar from MMD-Agent, PMX Editor
    // set default gravity 
    // some tweak for the simulation to match that of MikuMikuDance
    const float Scale = 10.f;
    const float EarthGravity = 9.8f;

    NumVar m_GravityAccel( "Application/Physics/Gravity Acceleration", EarthGravity, -100, 100, 1 );
    NumVar m_GravityX( "Application/Physics/Gravity X", 0, -1, 1, 0.1 );
    NumVar m_GravityY( "Application/Physics/Gravity Y", -1, -1, 1, 0.1 );
    NumVar m_GravityZ( "Application/Physics/Gravity Z", 0, -1, 1, 0.1 );

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

	std::mutex mutexJob;
	std::condition_variable condJob;
    float m_deltaT = 0.f;
	bool bStepJob = false;
	bool bExitJob = false;
	std::unique_ptr<std::thread> pJob;
    void JobFunc();

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

    void UpdateGravity();
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
        if (raycast.feature == btSoftBody::eFeature::Face)
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
#ifndef RELEASE
    PushProfilingMarker( Utility::MakeWStr(std::string(name)), nullptr );
#endif
}

void LeaveProfileZoneDefault()
{
#ifndef RELEASE
    PopProfilingMarker( nullptr );
#endif
}

btConstraintSolver* Physics::CreateSolverByType( SolverType t )
{
#if USE_BULLET_2_75
    return new btSequentialImpulseConstraintSolver();
#else
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
#endif
}

void Physics::Initialize( void )
{
    pJob.reset( new std::thread( JobFunc ) );

    BulletDebug::Initialize();
    PrimitiveBatch::Initialize();
#if !USE_BULLET_2_75
    btSetCustomEnterProfileZoneFunc(EnterProfileZoneDefault);
    btSetCustomLeaveProfileZoneFunc(LeaveProfileZoneDefault);
#endif

    Config = std::make_unique<btSoftBodyRigidBodyCollisionConfiguration>();
    Broadphase = std::make_unique<btDbvtBroadphase>();
    Dispatcher = std::make_unique<btCollisionDispatcher>( Config.get() );
    Solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    Solver.reset( CreateSolverByType( m_SolverType ) );
    DynamicsWorld = std::make_unique<btSoftRigidDynamicsWorld>( Dispatcher.get(), Broadphase.get(), Solver.get(), Config.get() );
    ASSERT( DynamicsWorld != nullptr );
    UpdateGravity();
    DynamicsWorld->getSolverInfo().m_solverMode = m_SolverMode;

    SoftBodyWorldInfo.m_broadphase = Broadphase.get();
    SoftBodyWorldInfo.m_dispatcher = Dispatcher.get();
    SoftBodyWorldInfo.m_gravity = DynamicsWorld->getGravity();
    SoftBodyWorldInfo.m_sparsesdf.Initialize();

    DebugDrawer = std::make_unique<BulletDebug::DebugDraw>();
    DebugDrawer->setDebugMode(
        // btIDebugDraw::DBG_DrawAabb |
        // btIDebugDraw::DBG_DrawConstraints |
        // btIDebugDraw::DBG_DrawConstraintLimits |
        btIDebugDraw::DBG_DrawWireframe
    );
    DynamicsWorld->setDebugDrawer( DebugDrawer.get() );
    auto pickCallback = []( btDynamicsWorld * world, btScalar timeStep ) {
        m_Picking.PickingPreTickCallback( world, timeStep );
    };
    DynamicsWorld->setInternalTickCallback( pickCallback, nullptr, true );
    g_DynamicsWorld = DynamicsWorld.get();

}

void Physics::JobFunc()
{
    while (true) 
    {
        {
			std::unique_lock<std::mutex> lk(mutexJob);
            condJob.wait( lk, [] { return bStepJob; } );
            if (bExitJob)
                break;
            UpdateGravity();
        #if !USE_BULLET_2_75
            DynamicsWorld->setLatencyMotionStateInterpolation( m_bInterpolation );
		#endif
            ASSERT( DynamicsWorld.get() != nullptr );
            DynamicsWorld->stepSimulation( m_deltaT, 2 );

            bStepJob = false;
        }
        condJob.notify_one();
	}
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

void Physics::Stop()
{
    {
        std::unique_lock<std::mutex> lk( mutexJob );
        bStepJob = true;
        bExitJob = true;
        condJob.notify_one();
    }
	pJob->join();
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
    std::unique_lock<std::mutex> lk( mutexJob );
    if (bStepJob) return;
    m_deltaT = deltaT;
    bStepJob = true;
	condJob.notify_one();
}

void Physics::UpdateGravity( void )
{
    DynamicsWorld->setGravity( btVector3( m_GravityX, m_GravityY, m_GravityZ ) * m_GravityAccel * Scale );
}

void Physics::UpdatePicking( D3D11_VIEWPORT MainViewport, const Math::BaseCamera& Camera )
{
    auto GetRayTo = [&]( float x, float y) {
        auto& invView = Camera.GetCameraToWorld();
        auto& proj = Camera.GetProjMatrix();
        auto p00 = proj.GetX().GetX();
        auto p11 = proj.GetY().GetY();
        float vx = (+2.f * x / MainViewport.Width - 1.f) / p00;
        float vy = (-2.f * y / MainViewport.Height + 1.f) / p11;
        Vector3 Dir(XMVector3TransformNormal( Vector3( vx, vy, -1 ), Matrix4(invView) ));
        return Normalize(Dir);
    };

    using namespace GameInput;
    static bool bMouseDrag = false;
    if (IsFirstReleased( DigitalInput::kMouse0 ))
    {
        bMouseDrag = false;
        Physics::ReleasePickBody();
    }
    if (IsFirstPressed( DigitalInput::kMouse0 ) || bMouseDrag)
    {
        float sx = (float)GetMousePosition( 0 ), sy = (float)GetMousePosition( 1 );
        Vector3 rayFrom = Camera.GetPosition();
        Vector3 rayTo = GetRayTo( sx, sy ) * Camera.GetFarClip() + rayFrom;
        if (!bMouseDrag)
        {
            Physics::PickBody( Convert( rayFrom ), Convert( rayTo ), Convert( Camera.GetForwardVec() ) );
            bMouseDrag = true;
        }
        else
        {
            Physics::MovePickBody( Convert( rayFrom ), Convert( rayTo ), Convert( Camera.GetForwardVec() ) );
        }
    }
}

void Physics::Wait()
{
    std::unique_lock<std::mutex> lk( mutexJob );
    condJob.wait( lk, [] { return !bStepJob; } );
}