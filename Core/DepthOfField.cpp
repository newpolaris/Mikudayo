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
#include "DepthOfField.h"
// #include "RootSignature.h"
#include "PipelineState.h"
#include "CommandContext.h"
#include "BufferManager.h"

namespace DepthOfField
{
	BoolVar Enable("Graphics/Depth of Field/Enable", false);
	BoolVar EnablePreFilter("Graphics/Depth of Field/PreFilter", true);
	BoolVar MedianFilter("Graphics/Depth of Field/Median Filter", true);
	BoolVar MedianAlpha("Graphics/Depth of Field/Median Alpha", false);
	NumVar FocalDepth("Graphics/Depth of Field/Focal Center", 0.1f, 0.0f, 1.0f, 0.01f);
	NumVar FocalRange("Graphics/Depth of Field/Focal Radius", 0.1f, 0.0f, 1.0f, 0.01f);
	NumVar ForegroundRange("Graphics/Depth of Field/FG Range", 100.0f, 10.0f, 1000.0f, 10.0f);
	NumVar AntiSparkleWeight("Graphics/Depth of Field/AntiSparkle", 1.0f, 0.0f, 10.0f, 1.0f);
	const char* DebugLabels[] = { "Off", "Foreground", "Background", "FG Alpha", "CoC" };
	EnumVar DebugMode("Graphics/Depth of Field/Debug Mode", 0, _countof(DebugLabels), DebugLabels);
	BoolVar DebugTiles("Graphics/Depth of Field/Debug Tiles", false);
	BoolVar ForceSlow("Graphics/Depth of Field/Force Slow Path", false);
	BoolVar ForceFast("Graphics/Depth of Field/Force Fast Path", false);

	ComputePSO s_DoFPass1CS;				// Responsible for classifying tiles (1st pass)
	ComputePSO s_DoFTilePassCS;				// Disperses tile info to its neighbors (3x3)
	ComputePSO s_DoFTilePassFixupCS;		// Searches for straggler tiles to "fixup"

	ComputePSO s_DoFPreFilterCS;			// Full pre-filter with variable focus
	ComputePSO s_DoFPreFilterFastCS;		// Pre-filter assuming near-constant focus
	ComputePSO s_DoFPreFilterFixupCS;		// Pass through colors for completely in focus tile

	ComputePSO s_DoFPass2CS;				// Perform full CoC convolution pass
	ComputePSO s_DoFPass2FastCS;			// Perform color-only convolution for near-constant focus
	ComputePSO s_DoFPass2FixupCS;			// Pass through colors again
	ComputePSO s_DoFPass2DebugCS;			// Full pass 2 shader with options for debugging

	ComputePSO s_DoFMedianFilterCS;			// 3x3 median filter to reduce fireflies
	ComputePSO s_DoFMedianFilterSepAlphaCS;	// 3x3 median filter to reduce fireflies (separate filter on alpha)
	ComputePSO s_DoFMedianFilterFixupCS;	// Pass through without performing median

	ComputePSO s_DoFCombineCS;				// Combine DoF blurred buffer with focused color buffer
	ComputePSO s_DoFCombineFastCS;			// Upsample DoF blurred buffer
	ComputePSO s_DoFDebugRedCS;				// Output red to entire tile for debugging
	ComputePSO s_DoFDebugGreenCS;			// Output green to entire tile for debugging
	ComputePSO s_DoFDebugBlueCS;			// Output blue to entire tile for debugging

	// IndirectArgsBuffer s_IndirectParameters;
}
