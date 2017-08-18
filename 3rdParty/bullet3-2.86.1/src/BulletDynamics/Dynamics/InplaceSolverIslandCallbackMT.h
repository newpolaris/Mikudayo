#pragma once

struct InplaceSolverIslandCallbackMt : public btSimulationIslandManagerMt::IslandCallback
{
	btContactSolverInfo*	m_solverInfo;
	btConstraintSolver*		m_solver;
	btIDebugDraw*			m_debugDrawer;
	btDispatcher*			m_dispatcher;

	InplaceSolverIslandCallbackMt(
		btConstraintSolver*	solver,
		btStackAlloc* stackAlloc,
		btDispatcher* dispatcher)
		:m_solverInfo(NULL),
		m_solver(solver),
		m_debugDrawer(NULL),
		m_dispatcher(dispatcher)
	{

	}

	InplaceSolverIslandCallbackMt& operator=(InplaceSolverIslandCallbackMt& other)
	{
		btAssert(0);
		(void)other;
		return *this;
	}

	SIMD_FORCE_INLINE void setup ( btContactSolverInfo* solverInfo, btIDebugDraw* debugDrawer)
	{
		btAssert(solverInfo);
		m_solverInfo = solverInfo;
		m_debugDrawer = debugDrawer;
	}


	virtual	void	processIsland( btCollisionObject** bodies,
                                   int numBodies,
                                   btPersistentManifold** manifolds,
                                   int numManifolds,
                                   btTypedConstraint** constraints,
                                   int numConstraints,
                                   int islandId
                                   )
	{
        m_solver->solveGroup( bodies,
                              numBodies,
                              manifolds,
                              numManifolds,
                              constraints,
                              numConstraints,
                              *m_solverInfo,
                              m_debugDrawer,
                              m_dispatcher
                              );
    }

};

