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

#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>

class CommandQueue
{
	friend class CommandListManager;
	friend class CommandContext;

public:
	CommandQueue();
	~CommandQueue();

	void Create(ID3D11Device* pDevice);
	void Shutdown();

private:

	uint64_t ExecuteCommandList(ID3D11CommandList* List);
};

class CommandListManager
{
	friend class CommandContext;

public:
	CommandListManager();
	~CommandListManager();

	void Create(ID3D11Device* pDevice);
	void Shutdown();

	// The CPU will wait for all command queues to empty (so that the GPU is idle)
	void IdleGPU(void)
	{
	}

private:

	ID3D11Device* m_Device;
};
