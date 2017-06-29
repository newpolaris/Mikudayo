//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "pch.h"
#include "SSAO.h"
#include "BufferManager.h"
#include "GraphicsCore.h"
#include "CommandContext.h"
#include "Camera.h"
#include "TemporalEffects.h"

using namespace Graphics;
using namespace Math;

namespace SSAO
{
    BoolVar Enable("Graphics/SSAO/Enable", true);
    BoolVar DebugDraw("Graphics/SSAO/Debug Draw", false);
    BoolVar AsyncCompute("Graphics/SSAO/Async Compute", false);
    BoolVar ComputeLinearZ("Graphics/SSAO/Always Linearize Z", true);
}
