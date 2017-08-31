#pragma once

#include <iostream>
#include <algorithm>
#include <regex>
#include <codecvt>
#include <locale>
#include "GameCore.h"
#include "GraphicsCore.h"
#include "PipelineState.h"
#include "CommandContext.h"
#include "GpuBuffer.h"
#include "InputLayout.h"
#include "ColorBuffer.h"
#include "BufferManager.h"
#include "Camera.h"
#include "CameraController.h"
#include "SamplerManager.h"
#include "GameInput.h"
#include "TextureManager.h"
#include "FileUtility.h"
#include "Utility.h"
#include "Math/BoundingBox.h"
#include "GeometryGenerator.h"
#include "boost/filesystem.hpp"

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

#pragma warning(push)
#pragma warning(disable: 4819)
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#pragma warning(pop)