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
#include "CommandListManager.h"

CommandListManager::CommandListManager() :
	m_Device(nullptr)
{
}

CommandListManager::~CommandListManager()
{
	Shutdown();
}

void CommandListManager::Shutdown()
{
}

void CommandListManager::Create(ID3D11Device* pDevice)
{
	ASSERT(pDevice != nullptr);

	m_Device = pDevice;
}
