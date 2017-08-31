#pragma once

class GraphicsContext;
class btSoftRigidDynamicsWorld;
struct btSoftBodyWorldInfo;

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

	extern btSoftRigidDynamicsWorld* g_DynamicsWorld;
    extern btSoftBodyWorldInfo* g_SoftBodyWorldInfo;

    void Initialize( void );
    void Shutdown( void );
    void Update( float deltaT );
    void Render( GraphicsContext& Context, const Math::Matrix4& WorldToClip );
};
