#pragma once

class btVector3;
class GraphicsContext;
class btDynamicsWorld;

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

    struct ProfileStatus
    {
        uint32_t NumIslands = 0;
        uint32_t NumCollisionObjects = 0;
        uint32_t NumManifolds = 0;
        uint32_t NumContacts = 0;
        uint32_t NumThread = 0;
        float InternalTimeStep = 0.f;
        float DispatchAllCollisionPairs = 0.f;
        float DispatchIslands = 0.f;
        float PredictUnconstrainedMotion = 0.f;
        float CreatePredictiveContacts = 0.f;
        float IntegrateTransforms = 0.f;
    };

	extern btDynamicsWorld* g_DynamicsWorld;

    void Initialize( void );
    void Shutdown( void );
    void Update( float deltaT );
    bool MovePickBody(const btVector3& From, const btVector3& To, const btVector3& Forward );
    bool PickBody( const btVector3& From, const btVector3& To, const btVector3& Forward );
    void ReleasePickBody();
    void Render( GraphicsContext& Context, const Math::Matrix4& ClipToWorld );
    void Profile( ProfileStatus& Status );
};
