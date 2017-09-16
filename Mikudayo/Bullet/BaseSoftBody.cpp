#include "stdafx.h"
#include "BaseSoftBody.h"
#include "Bullet/Physics.h"
#include "SoftBodyManager.h"

using namespace Physics;

namespace {

    struct TriangleFace
    {
        TriangleFace() {}
        TriangleFace( const btVector3& a, const btVector3& b,
            const btVector3& c, const btVector3& n);
        Vector3 Position[3];
        Vector3 Normal;
        Vector3 Center;
    };

    TriangleFace::TriangleFace( const btVector3& a, 
        const btVector3& b, const btVector3& c,
        const btVector3& n )
    {
        Position[0] = Convert(a);
        Position[1] = Convert(b);
        Position[2] = Convert(c);
        Normal = Convert(n);
        Center = (Position[0] + Position[1] + Position[2]) / 3;
    }

    AffineTransform TriPose( const TriangleFace& Face )
    {
        auto c = Face.Center;
        auto z = Face.Normal;
        auto y = Normalize( Face.Position[0] - c );
        auto x = Cross( y, z );
        return AffineTransform( x, y, z, c );
    }
}

BaseSoftBody::BaseSoftBody()
{
}

bool BaseSoftBody::Build( const SoftBodyInfo& Info )
{
    auto psb = Create(Info.Geometry);
    if (!psb)
        return false;

    auto& cfg = Info.Config;
    btSoftBody::Material* pm = psb->appendMaterial();
    pm->m_kLST = cfg.BendConst.kLST;
    pm->m_kAST = cfg.BendConst.kAST;
    pm->m_kVST = cfg.BendConst.kVST;
    pm->m_flags -= btSoftBody::fMaterial::DebugDraw;
    psb->generateBendingConstraints( cfg.BendConst.Distance, pm );
    psb->m_cfg.kVCF = cfg.kVCF; // Velocities correction factor (Baumgarte)
    psb->m_cfg.kDP = cfg.kDP; // Damping coefficient [0,1]
    psb->m_cfg.kDG = cfg.kDG; // Drag coefficient [0,+inf]
    psb->m_cfg.kLF = cfg.kLF; // Lift coefficient [0,+inf]
    psb->m_cfg.kPR = cfg.kPR;
    psb->m_cfg.kVC = cfg.kVC;
    psb->m_cfg.kDF = cfg.kDF;
    psb->m_cfg.kMT = cfg.kMT;
    psb->m_cfg.kCHR = cfg.kCHR; // Rigid contacts hardness [0,1]
    psb->m_cfg.kKHR = cfg.kKHR; // Kinetic contacts hardness [0,1]
    psb->m_cfg.kSHR = cfg.kSHR; // Soft contacts hardness [0,1]
    psb->m_cfg.kAHR = cfg.kAHR; // Anchors hardness [0,1]
    psb->m_cfg.viterations = cfg.viterations; // Velocities solver iterations
    psb->m_cfg.piterations = cfg.piterations; // Positions solver iterations
    psb->m_cfg.diterations = cfg.diterations; // Drift solver iterations
    psb->m_cfg.citerations = cfg.citerations; // Cluster solver iterations
    psb->m_cfg.collisions |= btSoftBody::fCollision::VF_SS;

    if (cfg.bRandomConst)
        psb->randomizeConstraints();
    psb->generateClusters( 3 );
    psb->scale( cfg.Scale );
    psb->setTotalMass( cfg.Mass, true );
    if (cfg.bSetPose)
        psb->setPose( cfg.bSetPoseVol, cfg.bSetPoseFrame );

    m_Body.swap(psb);

    return true;
}

void BaseSoftBody::GetSoftBodyPose( std::vector<AffineTransform>& Pose )
{
    if (!m_Body)
        return;
    uint32_t numFace = m_Body->m_faces.size();
    if (numFace > 1024)
        numFace = 1024;
    Pose.resize(numFace);
    for (uint32_t i = 0; i < numFace; i++)
    {
        const auto& f = m_Body->m_faces[i];
        TriangleFace face { 
            f.m_n[0]->m_x, f.m_n[1]->m_x,
            f.m_n[2]->m_x, f.m_normal 
        };
        Pose[i] = TriPose(face);
    }
}

void BaseSoftBody::GetSoftBodySkinning( 
    const std::vector<XMFLOAT3>& Position,
    std::vector<AffineTransform>& Pose,
    std::vector<XMUINT4>& Indices,
    std::vector<XMFLOAT4>& Weights )
{
    if (!m_Body)
        return;
    uint32_t numFace = m_Body->m_faces.size();
    if (numFace > 1024)
        numFace = 1024;
    Pose.resize(numFace);
    std::vector<TriangleFace> tri(numFace);
    for (uint32_t i = 0; i < numFace; i++)
    {
        const auto& f = m_Body->m_faces[i];
        TriangleFace face { 
            f.m_n[0]->m_x, f.m_n[1]->m_x,
            f.m_n[2]->m_x, f.m_normal 
        };
        Pose[i] = TriPose(face);
        tri[i] = face;
    }

    struct Pair {
        uint32_t ind;
        float dist;
    };

    Indices.resize(Position.size());
    Weights.resize(Position.size());

    for (uint32_t p = 0; p < Position.size(); p++ )
    {
        Pair dist[2] = { {0, FLT_MAX}, {0, FLT_MAX} };
        for (uint32_t i = 0; i < numFace; i++)
        {
            auto d = LengthSquare( tri[i].Center - Vector3(Position[p]) );
            if (d < dist[1].dist)
                dist[1] = { i, d };
            if (dist[1].dist < dist[0].dist)
                std::swap(dist[0], dist[1]);
        }
        float weightSum = 1.f + (dist[0].dist / dist[1].dist);
        float w0 = 1/weightSum;
        Indices[p] = { dist[0].ind, dist[1].ind, 0, 0 };
        Weights[p] = { w0, 1 - w0, 0, 0 };
    }
}

std::shared_ptr<btSoftBody> BaseSoftBody::Create( const SoftBodyGeometry& Geometry )
{
    ASSERT(Geometry.Positions.size() > 0);
    return std::shared_ptr<btSoftBody>(
        btSoftBodyHelpers::CreateFromTriMesh(
        *Physics::g_SoftBodyWorldInfo,
        reinterpret_cast<const btScalar*>(Geometry.Positions.data()),
        reinterpret_cast<const int*>(Geometry.Indices.data()),
        static_cast<int>(Geometry.Indices.size() / 3) ));
}

void BaseSoftBody::JoinWorld( btSoftRigidDynamicsWorld* value )
{
    value->addSoftBody( m_Body.get() );
}

void BaseSoftBody::LeaveWorld( btSoftRigidDynamicsWorld* value )
{
    value->removeSoftBody( m_Body.get() );
}
