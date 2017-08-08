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
#include "btBulletDynamicsCommon.h"
#include "BulletDebugDraw.h"

using namespace GameCore;
using namespace Graphics;
using namespace Math;

namespace Physics
{
    btBroadphaseInterface* broadphase = nullptr;
    btCollisionDispatcher* dispatcher = nullptr;
    btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
    btSequentialImpulseConstraintSolver* solver = nullptr;
    btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
    btCollisionShape* groundShape = nullptr;
    btCollisionShape* fallShape = nullptr;
    btDefaultMotionState* groundMotionState = nullptr;
    btRigidBody* groundRigidBody = nullptr;
    btDefaultMotionState* fallMotionState = nullptr;
    btRigidBody* fallRigidBody = nullptr;
    BulletDebugDraw* debugDrawer = nullptr;

    void Initialize( void );
    void Shutdown( void );
    void Update( float deltaT );
    void Render( GraphicsContext& Context, const Matrix4& ClipToWorld );
};

void Physics::Initialize( void )
{
    BulletDebug::Initialize();

    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher( collisionConfiguration );
    solver = new btSequentialImpulseConstraintSolver;
    dynamicsWorld = new btDiscreteDynamicsWorld( dispatcher, broadphase, solver, collisionConfiguration );
    dynamicsWorld->setGravity( btVector3( 0, -10, 0 ) );

    groundMotionState = new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ), btVector3( 0, -1, 0 ) ) );
    groundShape = new btStaticPlaneShape( btVector3( 0, 1, 0 ), 1 );
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI( 0, groundMotionState, groundShape, btVector3( 0, 0, 0 ) );
    groundRigidBody = new btRigidBody( groundRigidBodyCI );
    dynamicsWorld->addRigidBody( groundRigidBody );

    fallMotionState = new btDefaultMotionState( btTransform( btQuaternion( 0, 0, 0, 1 ), btVector3( 0, 50, 0 ) ) );
    btScalar mass = 1;
    btVector3 fallInertia( 0, 0, 0 );
    fallShape = new btSphereShape( 1 );
    fallShape->calculateLocalInertia( mass, fallInertia );
    btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI( mass, fallMotionState, fallShape, fallInertia );
    fallRigidBody = new btRigidBody( fallRigidBodyCI );
    dynamicsWorld->addRigidBody( fallRigidBody );
    debugDrawer = new BulletDebugDraw();
    debugDrawer->setDebugMode(
        btIDebugDraw::DBG_DrawConstraints |
        btIDebugDraw::DBG_DrawConstraintLimits |
        btIDebugDraw::DBG_DrawWireframe |
        btIDebugDraw::DBG_DrawAabb );
    dynamicsWorld->setDebugDrawer( debugDrawer );
}

void Physics::Shutdown( void )
{
    dynamicsWorld->removeRigidBody( fallRigidBody );
    delete fallRigidBody->getMotionState();
    delete fallRigidBody;

    dynamicsWorld->removeRigidBody( groundRigidBody );
    delete groundRigidBody->getMotionState();
    delete groundRigidBody;
    delete fallShape;
    delete groundShape;
    delete debugDrawer;
    delete dynamicsWorld;
    delete solver;
    delete collisionConfiguration;
    delete dispatcher;
    delete broadphase;

    BulletDebug::Shutdown();
}

void Physics::Update( float deltaT )
{
    dynamicsWorld->stepSimulation( deltaT );
    {
        btTransform trans;
        fallRigidBody->getMotionState()->getWorldTransform( trans );
    }
}

void Physics::Render( GraphicsContext& Context, const Matrix4& ClipToWorld )
{
    dynamicsWorld->debugDrawWorld();
    debugDrawer->flush( Context, ClipToWorld );
}

class ModelViewer : public GameCore::IGameApp
{
public:
	ModelViewer()
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
    Matrix4 m_ViewProjMatrix;
    D3D11_VIEWPORT m_MainViewport;
    D3D11_RECT m_MainScissor;

    GraphicsPSO m_DepthPSO;
    GraphicsPSO m_CutoutDepthPSO;
    GraphicsPSO m_ModelPSO;
};

CREATE_APPLICATION( ModelViewer )

void ModelViewer::Startup( void )
{
    Physics::Initialize();

    const Vector3 eye = Vector3(0.0f, 10.0f, 10.0f);
    m_Camera.SetEyeAtUp( eye, Vector3(kZero), Vector3(kYUnitVector) );
    m_CameraController.reset(new CameraController(m_Camera, Vector3(kYUnitVector)));
}

void ModelViewer::Cleanup( void )
{
    Physics::Shutdown();
}

void ModelViewer::Update( float deltaT )
{
    Physics::Update( deltaT );
    m_CameraController->Update( deltaT );
    m_ViewProjMatrix = m_Camera.GetViewProjMatrix();
}

void ModelViewer::RenderScene( void )
{
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");
    gfxContext.ClearColor( g_SceneColorBuffer );
    gfxContext.ClearDepth( g_SceneDepthBuffer );
    gfxContext.Finish();
}

void ModelViewer::RenderUI( GraphicsContext& Context )
{
    Physics::Render( Context, m_ViewProjMatrix );
}
