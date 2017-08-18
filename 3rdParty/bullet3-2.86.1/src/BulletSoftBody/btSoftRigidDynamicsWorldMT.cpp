#include "btSoftRigidDynamicsWorldMT.h"

//collision detection
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/BroadphaseCollision/btSimpleBroadphase.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"
#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "BulletDynamics/Dynamics/btSimulationIslandManagerMt.h"
#include "LinearMath/btTransformUtil.h"
#include "LinearMath/btQuickprof.h"

//rigidbody & constraints
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "BulletDynamics/ConstraintSolver/btContactSolverInfo.h"
#include "BulletDynamics/ConstraintSolver/btTypedConstraint.h"
#include "BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h"
#include "BulletDynamics/ConstraintSolver/btHingeConstraint.h"
#include "BulletDynamics/ConstraintSolver/btConeTwistConstraint.h"
#include "BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.h"
#include "BulletDynamics/ConstraintSolver/btGeneric6DofSpring2Constraint.h"
#include "BulletDynamics/ConstraintSolver/btSliderConstraint.h"
#include "BulletDynamics/ConstraintSolver/btContactConstraint.h"


#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"


#include "BulletDynamics/Dynamics/btActionInterface.h"
#include "BulletDynamics/Dynamics/InplaceSolverIslandCallbackMt.h"
#include "LinearMath/btQuickprof.h"
#include "LinearMath/btMotionState.h"
#include "LinearMath/btSerializer.h"


btSoftRigidDynamicsWorldMT::btSoftRigidDynamicsWorldMT(btDispatcher* dispatcher,btBroadphaseInterface* pairCache,btConstraintSolver* constraintSolver, btCollisionConfiguration* collisionConfiguration)
: btSoftRigidDynamicsWorld(dispatcher,pairCache,constraintSolver,collisionConfiguration)
{
	if (m_ownsIslandManager)
	{
		m_islandManager->~btSimulationIslandManager();
		btAlignedFree( m_islandManager);
	}
    {
		void* mem = btAlignedAlloc(sizeof(InplaceSolverIslandCallbackMt),16);
		m_solverIslandCallbackMt = new (mem) InplaceSolverIslandCallbackMt (m_constraintSolver, 0, dispatcher);
    }
	{
		void* mem = btAlignedAlloc(sizeof(btSimulationIslandManagerMt),16);
		btSimulationIslandManagerMt* im = new (mem) btSimulationIslandManagerMt();
        m_islandManager = im;
        im->setMinimumSolverBatchSize( m_solverInfo.m_minimumSolverBatchSize );
	}
}


btSoftRigidDynamicsWorldMT::~btSoftRigidDynamicsWorldMT()
{
	if (m_solverIslandCallbackMt)
	{
		m_solverIslandCallbackMt->~InplaceSolverIslandCallbackMt();
		btAlignedFree(m_solverIslandCallbackMt);
	}
	if (m_ownsConstraintSolver)
	{
		m_constraintSolver->~btConstraintSolver();
		btAlignedFree(m_constraintSolver);
	}
}


void	btSoftRigidDynamicsWorldMT::solveConstraints(btContactSolverInfo& solverInfo)
{
	BT_PROFILE("solveConstraints");

	m_solverIslandCallbackMt->setup(&solverInfo, getDebugDrawer());
	m_constraintSolver->prepareSolve(getCollisionWorld()->getNumCollisionObjects(), getCollisionWorld()->getDispatcher()->getNumManifolds());

	/// solve all the constraints for this island
    btSimulationIslandManagerMt* im = static_cast<btSimulationIslandManagerMt*>(m_islandManager);
    im->buildAndProcessIslands( getCollisionWorld()->getDispatcher(), getCollisionWorld(), m_constraints, m_solverIslandCallbackMt );

	m_constraintSolver->allSolved(solverInfo, m_debugDrawer);
}



