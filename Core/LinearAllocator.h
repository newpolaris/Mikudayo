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
// Description:  This is a dynamic graphics memory allocator for DX12.  It's designed to work in concert
// with the CommandContext class and to do so in a thread-safe manner.  There may be many command contexts,
// each with its own linear allocators.  They act as windows into a global memory pool by reserving a
// context-local memory page.  Requesting a new page is done in a thread-safe manner by guarding accesses
// with a mutex lock.
//
// When a command context is finished, it will receive a fence ID that indicates when it's safe to reclaim
// used resources.  The CleanupUsedPages() method must be invoked at this time so that the used pages can be
// scheduled for reuse after the fence has cleared.

#pragma once

#include "GpuResource.h"
#include "Mapping.h"
#include <vector>
#include <queue>
#include <mutex>

// Constant blocks must be multiples of 16 constants @ 16 bytes each
#define DEFAULT_ALIGN 256

struct DynAlloc
{
	DynAlloc( GpuResource& BaseResource, size_t ThisOffset, size_t ThisSize )
		: Buffer(BaseResource), DataPtr(nullptr)
	{
		Handle = reinterpret_cast<D3D11_BUFFER_HANDLE>( BaseResource.GetResource() );
		FirstConstant = static_cast<UINT>(ThisOffset / 16);
		NumConstants = static_cast<UINT>(ThisSize / 16);
	}
	D3D11_BUFFER_HANDLE Handle;
	GpuResource& Buffer;	// The D3D buffer associated with this memory.
	UINT FirstConstant;
	UINT NumConstants;
	void* DataPtr;
};

class LinearAllocationPage : public GpuResource
{
public:
	LinearAllocationPage( Microsoft::WRL::ComPtr<ID3D11Buffer> resource );
	~LinearAllocationPage() {}

	void Map( ID3D11_CONTEXT* pContext );
	void Unmap( ID3D11_CONTEXT* pContext );

	void* m_CpuVirtualAddress;
};

enum LinearAllocatorType
{
	kInvalidAllocator = -1,

	kGpuExclusive = 0,		// DEFAULT   GPU-writeable (via UAV)
	kCpuWritable = 1,		// UPLOAD CPU-writeable (but write combined)

	kNumAllocatorTypes
};

enum
{
	kGpuAllocatorPageSize = 0x10000,	// 64K
	kCpuAllocatorPageSize = 0x200000	// 2MB
};

class LinearAllocatorPageManager
{
public:

	LinearAllocatorPageManager( LinearAllocatorType Type );
	LinearAllocationPage* RequestPage( void );
	LinearAllocationPage* CreateNewPage( size_t PageSize = 0 );

	// Discarded pages will get recycled.  This is for fixed size pages.
	void DiscardPages( uint64_t FenceID, ID3D11_CONTEXT* Context, const std::vector<LinearAllocationPage*>& Pages );

	// Freed pages will be destroyed once their fence has passed.  This is for single-use,
	// "large" pages.
	void FreeLargePages( uint64_t FenceID, ID3D11_CONTEXT* Context, const std::vector<LinearAllocationPage*>& Pages );

	void Destroy( void ) { m_PagePool.clear(); }

private:

	LinearAllocatorType m_AllocationType;
	std::vector<std::unique_ptr<LinearAllocationPage> > m_PagePool;
	std::queue<std::pair<uint64_t, LinearAllocationPage*> > m_RetiredPages;
	std::queue<std::pair<uint64_t, LinearAllocationPage*> > m_DeletionQueue;
	std::queue<LinearAllocationPage*> m_AvailablePages;
	std::mutex m_Mutex;
};

class LinearAllocator
{
public:

	LinearAllocator( LinearAllocatorType Type ) : 
		m_AllocationType( Type ), m_PageSize( 0 ), m_CurOffset( ~(size_t)0 ), 
		m_CurPage( nullptr ), m_Context( nullptr )
	{
		ASSERT(Type > kInvalidAllocator && Type < kNumAllocatorTypes);
		m_PageSize = (Type == kGpuExclusive ? kGpuAllocatorPageSize : kCpuAllocatorPageSize);
	}

	~LinearAllocator() { m_Context = nullptr; }

	void Initialize( ID3D11_CONTEXT* Context ) 
	{
		m_Context = Context;
	}

	DynAlloc Allocate( size_t SizeInBytes, size_t Alignment = DEFAULT_ALIGN );

	void CleanupUsedPages( uint64_t FenceID );

	static void DestroyAll( void )
	{
		sm_PageManager[0].Destroy();
		sm_PageManager[1].Destroy();
	}

private:

	DynAlloc AllocateLargePage( size_t SizeInBytes );

	static LinearAllocatorPageManager sm_PageManager[2];

	ID3D11_CONTEXT* m_Context;

	LinearAllocatorType m_AllocationType;
	size_t m_PageSize;
	size_t m_CurOffset;
	LinearAllocationPage* m_CurPage;
	std::vector<LinearAllocationPage*> m_RetiredPages;
	std::vector<LinearAllocationPage*> m_LargePageList;
};


