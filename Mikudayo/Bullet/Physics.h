#pragma once

class btVector3;
class GraphicsContext;
class btSoftRigidDynamicsWorld;
struct btSoftBodyWorldInfo;

// Bullet Physcis
#pragma warning(push)
#pragma warning(disable: 4100)
#pragma warning(disable: 4456)
#pragma warning(disable: 4702)
#pragma warning(disable: 4819)
#define BT_THREADSAFE 1
#define BT_NO_SIMD_OPERATOR_OVERLOADS 1
#include "btBulletDynamicsCommon.h"
#include "LinearMath/btThreads.h"
#include "LinearMath/btQuickprof.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletDynamics/MLCPSolvers/btSolveProjectedGaussSeidel.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "BulletDynamics/ConstraintSolver/btNNCGConstraintSolver.h"
#include "BulletDynamics/MLCPSolvers/btMLCPSolver.h"
#include "BulletDynamics/MLCPSolvers/btSolveProjectedGaussSeidel.h"
#include "BulletDynamics/MLCPSolvers/btDantzigSolver.h"
#include "BulletDynamics/MLCPSolvers/btLemkeSolver.h"
#pragma warning(pop)

namespace Math
{
    class Matrix4;
}

namespace Physics
{
    using namespace Math;
    enum SolverType
    {
        SOLVER_TYPE_SEQUENTIAL_IMPULSE,
        SOLVER_TYPE_NNCG,
        SOLVER_TYPE_MLCP_PGS,
        SOLVER_TYPE_MLCP_DANTZIG,
        SOLVER_TYPE_MLCP_LEMKE,
        SOLVER_TYPE_COUNT
    };

	extern btSoftRigidDynamicsWorld* g_DynamicsWorld;
    extern btSoftBodyWorldInfo* g_SoftBodyWorldInfo;

    void Initialize( void );
    void Shutdown( void );
    void Update( float deltaT );
    bool MovePickBody(const btVector3& From, const btVector3& To, const btVector3& Forward );
    bool PickBody( const btVector3& From, const btVector3& To, const btVector3& Forward );
    void ReleasePickBody();
    void Render( GraphicsContext& Context, const Matrix4& WorldToClip );
};
