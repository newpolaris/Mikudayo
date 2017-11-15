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

#pragma once

#include "pch.h"
#include "EngineTuning.h"

class ComputeContext;

namespace PostEffects
{
    extern BoolVar EnableFXAA;
    extern BoolVar BloomEnable;

    void Initialize( void );
    void Shutdown( void );
    void Render( void );

    // Copy the contents of the post effects buffer onto the main scene buffer
    void CopyBackPostBuffer( ComputeContext& Context );
}
