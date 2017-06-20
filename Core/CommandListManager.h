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

enum ECOMMAND_LIST_TYPE
{
	kCOMMAND_LIST_TYPE_DIRECT	= 0,
	kCOMMAND_LIST_TYPE_BUNDLE	= 1,
	kCOMMAND_LIST_TYPE_COMPUTE	= 2,
	kCOMMAND_LIST_TYPE_COPY	= 3
};

class CommandQueue
{
	friend class CommandListManager;
	friend class CommandContext;

public:
	CommandQueue(ECOMMAND_LIST_TYPE Type);
	~CommandQueue();

	void Create(ID3D11Device* pDevice);
	void Shutdown();

private:

	uint64_t ExecuteCommandList(ID3D11CommandList* List);

	const ECOMMAND_LIST_TYPE m_Type;
};

class CommandListManager
{
	friend class CommandContext;

public:
	CommandListManager();
	~CommandListManager();

	void Create(ID3D11Device* pDevice);
	void Shutdown();

	CommandQueue& GetQueue(ECOMMAND_LIST_TYPE Type = kCOMMAND_LIST_TYPE_DIRECT)
	{
		switch (Type)
		{
		case kCOMMAND_LIST_TYPE_COMPUTE: return m_ComputeQueue;
		case kCOMMAND_LIST_TYPE_COPY: return m_CopyQueue;
		default: return m_GraphicsQueue;
		}
	}

	// Test to see if a fence has already been reached
	bool IsFenceComplete(uint64_t FenceValue)
	{
		(FenceValue);
		return true;
	}

	// The CPU will wait for all command queues to empty (so that the GPU is idle)
	void IdleGPU(void)
	{
	}

private:

	ID3D11Device* m_Device;

	CommandQueue m_GraphicsQueue;
	CommandQueue m_ComputeQueue;
	CommandQueue m_CopyQueue;
};
