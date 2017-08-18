#pragma once

#include "btSoftRigidDynamicsWorld.h"

struct InplaceSolverIslandCallbackMt;

ATTRIBUTE_ALIGNED16(class) btSoftRigidDynamicsWorldMT : public btSoftRigidDynamicsWorld
{
protected:
    InplaceSolverIslandCallbackMt* m_solverIslandCallbackMt;

    virtual void	solveConstraints(btContactSolverInfo& solverInfo);

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btSoftRigidDynamicsWorldMT(btDispatcher* dispatcher,btBroadphaseInterface* pairCache,btConstraintSolver* constraintSolver,btCollisionConfiguration* collisionConfiguration);
    virtual ~btSoftRigidDynamicsWorldMT();
};
