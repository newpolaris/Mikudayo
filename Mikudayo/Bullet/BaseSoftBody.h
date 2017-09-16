#pragma once

#include <memory>

class btSoftBody;
class btSoftRigidDynamicsWorld;
struct SoftBodyInfo;
struct SoftBodyGeometry;

namespace Physics
{
    class BaseSoftBody
    {
    public:

        BaseSoftBody( void );

        bool Build( const SoftBodyInfo& Info );
        void GetSoftBodyPose();
        void GetSoftBodyPose( const std::vector<XMFLOAT3>& Position,
            std::vector<XMUINT4>& indices, std::vector<XMFLOAT4>& weights );
        void JoinWorld( btSoftRigidDynamicsWorld* value );
        void LeaveWorld( btSoftRigidDynamicsWorld* value );
        void syncLocalTransform();

    protected:

        std::shared_ptr<btSoftBody> Create( const SoftBodyGeometry& Geometry );

        std::shared_ptr<btSoftBody> m_Body;
    };
}
