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
// Author(s):  James Stanard
//             Alex Nankervis
//

#include "pch.h"
#include "LinearAllocator.h"
#include "GraphicsCore.h"
#include "CommandListManager.h"
#include <thread>

using namespace Graphics;
using namespace std;

LinearAllocatorPageManager::LinearAllocatorPageManager( LinearAllocatorType Type ) : m_AllocationType( Type )
{
}

LinearAllocatorPageManager LinearAllocator::sm_PageManager[2] = { kGpuExclusive, kCpuWritable };

LinearAllocationPage* LinearAllocatorPageManager::RequestPage()
{
	lock_guard<mutex> LockGuard(m_Mutex);

	while (!m_RetiredPages.empty() && g_CommandManager.IsFenceComplete(m_RetiredPages.front().first))
	{
		m_AvailablePages.push(m_RetiredPages.front().second);
		m_RetiredPages.pop();
	}

	LinearAllocationPage* PagePtr = nullptr;

	if (!m_AvailablePages.empty())
	{
		PagePtr = m_AvailablePages.front();
		m_AvailablePages.pop();
	}
	else
	{
		PagePtr = CreateNewPage();
		m_PagePool.emplace_back(PagePtr);
	}

	return PagePtr;
}

void LinearAllocatorPageManager::DiscardPages( uint64_t FenceValue, ID3D11DeviceContext3* Context, const vector<LinearAllocationPage*>& UsedPages )
{
	lock_guard<mutex> LockGuard(m_Mutex);
	for (auto page : UsedPages) 
	{
		page->Unmap( Context );
		m_RetiredPages.push(make_pair(FenceValue, page));
	}
}

void LinearAllocatorPageManager::FreeLargePages( uint64_t FenceValue, ID3D11DeviceContext3* Context, const vector<LinearAllocationPage*>& LargePages )
{
	lock_guard<mutex> LockGuard(m_Mutex);

	while (!m_DeletionQueue.empty() && g_CommandManager.IsFenceComplete(m_DeletionQueue.front().first))
	{
		delete m_DeletionQueue.front().second;
		m_DeletionQueue.pop();
	}

	for (auto page : LargePages)
	{
		page->Unmap( Context );
		m_DeletionQueue.push(make_pair(FenceValue, page));
	}
}

LinearAllocationPage* LinearAllocatorPageManager::CreateNewPage( size_t PageSize )
{
    UINT BindFlags;
	UINT CpuFlag = 0;
	size_t Width = 0;
	D3D11_USAGE Usage;
	if (m_AllocationType == kGpuExclusive)
	{
		Usage = D3D11_USAGE_DEFAULT;
		Width = PageSize == 0 ? kGpuAllocatorPageSize : PageSize;
		BindFlags = D3D11_BIND_CONSTANT_BUFFER | D3D11_BIND_UNORDERED_ACCESS;
	}
	else
	{
		Usage = D3D11_USAGE_DYNAMIC;
		CpuFlag = D3D11_CPU_ACCESS_WRITE;
		Width = PageSize == 0 ? kCpuAllocatorPageSize : PageSize;
		BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	}

	D3D11_BUFFER_DESC Desc;
	Desc.ByteWidth = static_cast<UINT>(Width);
	Desc.BindFlags = BindFlags;
	Desc.MiscFlags = 0;
	Desc.StructureByteStride = 0;
	Desc.Usage = Usage;
	Desc.CPUAccessFlags = CpuFlag;

	Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer;
	ASSERT_SUCCEEDED( g_Device->CreateBuffer( &Desc, nullptr, Buffer.GetAddressOf() ) );
	SetName( Buffer, L"LinearAllocator Page" );

	return new LinearAllocationPage(Buffer);
}

void LinearAllocator::CleanupUsedPages( uint64_t FenceID )
{
	if (m_CurPage == nullptr)
		return;

	m_RetiredPages.push_back(m_CurPage);
	m_CurPage = nullptr;
	m_CurOffset = 0;

	sm_PageManager[m_AllocationType].DiscardPages(FenceID, m_Context, m_RetiredPages);
	m_RetiredPages.clear();

	sm_PageManager[m_AllocationType].FreeLargePages(FenceID, m_Context, m_LargePageList);
	m_LargePageList.clear();
}

DynAlloc LinearAllocator::Allocate( size_t SizeInBytes, size_t Alignment )
{
	const size_t AlignmentMask = Alignment - 1;

	// Assert that it's a power of two.
	ASSERT( (AlignmentMask & Alignment) == 0 );

	// Align the allocation
	const size_t AlignedSize = Math::AlignUpWithMask(SizeInBytes, AlignmentMask);

	if ( AlignedSize > m_PageSize )
		return AllocateLargePage( AlignedSize );

	m_CurOffset = Math::AlignUp( m_CurOffset, Alignment );

	if (m_CurOffset + AlignedSize > m_PageSize)
	{
		ASSERT(m_CurPage != nullptr);
		m_RetiredPages.push_back(m_CurPage);
		m_CurPage = nullptr;
	}

	if (m_CurPage == nullptr)
	{
		m_CurPage = sm_PageManager[m_AllocationType].RequestPage();
		m_CurPage->Map( m_Context );
		m_CurOffset = 0;
	}

	DynAlloc ret( *m_CurPage, m_CurOffset, AlignedSize );
	ret.DataPtr = (uint8_t*)m_CurPage->m_CpuVirtualAddress + m_CurOffset;

	m_CurOffset += AlignedSize;

	return ret;
}

DynAlloc LinearAllocator::AllocateLargePage(size_t SizeInBytes)
{
	LinearAllocationPage* OneOff = sm_PageManager[m_AllocationType].CreateNewPage(SizeInBytes);
	m_LargePageList.push_back(OneOff);

	DynAlloc ret(*OneOff, 0, SizeInBytes);
	OneOff->Map( m_Context );
	ret.DataPtr = OneOff->m_CpuVirtualAddress;

	return ret;
}

LinearAllocationPage::LinearAllocationPage( Microsoft::WRL::ComPtr<ID3D11Buffer> resource ) : 
	GpuResource(), m_CpuVirtualAddress(nullptr)
{
	m_pResource.Swap( resource );
}

void LinearAllocationPage::Map( ID3D11DeviceContext3* pContext )
{
	D3D11_MAPPED_SUBRESOURCE MapData = {};
	ASSERT_SUCCEEDED( pContext->Map(
		GetResource(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&MapData ) );
	m_CpuVirtualAddress = MapData.pData;
}

void LinearAllocationPage::Unmap( ID3D11DeviceContext3* pContext )
{
	pContext->Unmap( GetResource(), 0 );
	m_CpuVirtualAddress = nullptr;
}
