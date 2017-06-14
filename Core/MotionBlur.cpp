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
#include "MotionBlur.h"
// #include "Camera.h"
#include "BufferManager.h"
#include "GraphicsCore.h"
#include "CommandContext.h"
// #include "Camera.h"
// #include "PostEffects.h"
// #include "SystemTime.h"

#include "CompiledShaders/ScreenQuadVS.h"

using namespace Graphics;
using namespace Math;
using namespace TemporalAA;

namespace TemporalAA
{
	BoolVar Enable("Graphics/AA/TAA/Enable", false);
	BoolVar Accumulate("Graphics/AA/TAA/Accumulate", true);
	NumVar TemporalMaxLerp("Graphics/AA/TAA/Blend Factor", 1.0f, 0.0f, 1.0f, 0.01f);
	ExpVar TemporalSpeedLimit("Graphics/AA/TAA/Speed Limit", 32.0f, 1.0f, 1024.0f, 1.0f);

	BoolVar TriggerReset("Graphics/AA/TAA/Reset", false);

	// Disabled blur but with temporal anti-aliasing that requires a velocity buffer
	ComputePSO s_TemporalBlendCS;
	ComputePSO s_BoundNeighborhoodCS;
}

namespace MotionBlur
{
	BoolVar Enable("Graphics/Motion Blur/Enable", false);

	ComputePSO s_CameraMotionBlurPrePassCS[2];
	ComputePSO s_MotionBlurPrePassCS;
	ComputePSO s_MotionBlurFinalPassCS;
	GraphicsPSO s_MotionBlurFinalPassPS;
	ComputePSO s_CameraVelocityCS[2];
}
