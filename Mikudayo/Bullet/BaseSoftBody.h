#pragma once

#include <memory>
#include "Math/Transform.h"

class btSoftBody;
class btSoftRigidDynamicsWorld;
struct SoftBodyInfo;
struct SoftBodyGeometry;

namespace Physics
{
    using Math::AffineTransform;

    class BaseSoftBody
    {
    public:

        BaseSoftBody( void );

        bool Build( const SoftBodyInfo& Info );
        void GetSoftBodyPose( std::vector<AffineTransform>& Pose );
        void GetSoftBodySkinning( const std::vector<XMFLOAT3>& Position, 
            std::vector<AffineTransform>& Pose,
            std::vector<XMUINT4>& Indices, std::vector<XMFLOAT4>& Weights );
        void JoinWorld( btSoftRigidDynamicsWorld* value );
        void LeaveWorld( btSoftRigidDynamicsWorld* value );
        void syncLocalTransform();

    protected:

        std::shared_ptr<btSoftBody> Create( const SoftBodyGeometry& Geometry );

        std::shared_ptr<btSoftBody> m_Body;
    };
}
