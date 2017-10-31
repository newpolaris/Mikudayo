#pragma once

#include <memory>
#include "Bullet/LinearMath.h"

class btSoftBody;
namespace Physics
{
    class BaseSoftBody;
}

struct SoftBodyConfig
{
	btScalar kVCF = 1.0f;
	btScalar kDP = 0.0f;
	btScalar kDG = 0.0f;
	btScalar kLF = 0.0f;
	btScalar kPR = 0.0f;
	btScalar kVC = 0.0f;
	btScalar kDF = 0.0f;
	btScalar kMT = 0.0f;
	btScalar kCHR = 1.0f;
	btScalar kKHR = 0.1f;
	btScalar kSHR = 1.0f;
	btScalar kAHR = 0.7f;
	
	int viterations = 0;
	int piterations = 1;
	int diterations = 0;
	int citerations = 4;

	btScalar Mass = 10.0f;
	bool bSetPose = false;
	bool bSetPoseVol = false;
	bool bSetPoseFrame = false;
	bool bRandomConst = false;
    btVector3 Scale = btVector3(1, 1, 1);

	struct GenBending
	{
		int Distance = 0;
		btScalar kLST = 1.0f;
		btScalar kAST = 1.0f;
		btScalar kVST = 1.0f;
	} BendConst;
};

struct SoftBodyGeometry
{
    std::vector<XMFLOAT3> Positions;
    std::vector<uint32_t> Indices;
};

struct SoftBodyInfo
{
    SoftBodyConfig Config;
    SoftBodyGeometry Geometry;
};

struct SoftBodySetting
{
    std::string Name;
    std::wstring Path;
    SoftBodyConfig Config;
};

namespace SoftBodyManager
{
    std::shared_ptr<Physics::BaseSoftBody> GetInstance( const std::string& Name );
    bool Load( const SoftBodySetting& SBSetting );
    void Initialize();
    void Shutdown();
}
