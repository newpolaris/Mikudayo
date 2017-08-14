#pragma once

class GraphicsContext;
class btDynamicsWorld;

namespace Math
{
    class Matrix4;
}

namespace Physics
{
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
    void Render( GraphicsContext& Context, const Math::Matrix4& ClipToWorld );
    void Profile( ProfileStatus& Status );
};
