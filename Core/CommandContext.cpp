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
#include "CommandContext.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "GraphicsCore.h"
#include "InputLayout.h"
#include "BlendState.h"
#include "Shader.h"
#include "GpuBuffer.h"
#include "EngineProfiling.h"

using namespace Graphics;

void ContextManager::DestroyAllContexts( void )
{
	sm_ContextPool.clear();
}

CommandContext* ContextManager::AllocateContext(ContextType Type)
{
	std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);

	auto& AvailableContexts = sm_AvailableContexts[Type];

	CommandContext* ret = nullptr;
	if (AvailableContexts.empty())
	{
		ret = nullptr;
		switch (Type)
		{
		case ContextType::kComputeContext:
			ret = new ComputeContext();
			break;
		case ContextType::kGraphicsContext:
			ret = new GraphicsContext();
			break;
		}

		sm_ContextPool.emplace_back(ret);
		ASSERT( sm_ContextPool.size() < 100, L"Forget to free context?" );
		ret->Initialize();
	}
	else
	{
		ret = AvailableContexts.front();
		AvailableContexts.pop();
		ret->Reset();
	}
	ASSERT(ret != nullptr);

	ASSERT(ret->m_Type == Type);

	return ret;
}

void ContextManager::FreeContext(CommandContext* UsedContext)
{
	ASSERT(UsedContext != nullptr);
	std::lock_guard<std::mutex> LockGuard( sm_ContextAllocationMutex );
	sm_AvailableContexts[UsedContext->m_Type].push(UsedContext);
}

std::mutex CommandContext::sm_ContextMutex;

void CommandContext::DestroyAllContexts(void)
{
	LinearAllocator::DestroyAll();
#ifdef GRAPHICS_DEBUG
	ConstantBufferAllocator::DestroyAll();
#endif
	g_ContextManager.DestroyAllContexts();
}

ComputeContext::ComputeContext() : CommandContext( kComputeContext )
{
}

GraphicsContext::GraphicsContext() : CommandContext( kGraphicsContext )
{
}

CommandContext& CommandContext::Begin( ContextType Type, const std::wstring ID )
{
	CommandContext* NewContext = g_ContextManager.AllocateContext( Type );
	NewContext->SetID( ID );
	if (ID.length() > 0)
	    EngineProfiling::BeginBlock(ID, NewContext);
	return *NewContext;
}

ComputeContext& ComputeContext::Begin( const std::wstring& ID )
{
    CommandContext& NewContext = CommandContext::Begin( kComputeContext, ID );
    return NewContext.GetComputeContext();
}

uint64_t CommandContext::Flush(bool WaitForCompletion)
{
	return 0;
}

uint64_t CommandContext::Finish( bool WaitForCompletion )
{
	if (m_ID.length() > 0)
	    EngineProfiling::EndBlock(this);

	Flush( WaitForCompletion );

	ComPtr<ID3D11CommandList> CommandList;
	m_CommandList->FinishCommandList( FALSE, &CommandList );

    {
        std::lock_guard<std::mutex> LockGuard(sm_ContextMutex);
        g_Context->ExecuteCommandList( CommandList.Get(), FALSE );
    }

	uint64_t FenceValue = 0;
	m_CpuLinearAllocator.CleanupUsedPages(FenceValue);
	m_GpuLinearAllocator.CleanupUsedPages(FenceValue);

	g_ContextManager.FreeContext( this );

	return 0;
}

CommandContext::CommandContext(ContextType Type) :
	m_Type(Type),
	m_CpuLinearAllocator(kCpuWritable),
	m_GpuLinearAllocator(kGpuExclusive),
	m_CommandList(nullptr),
	m_OwningManager(nullptr)
{
}

void CommandContext::Reset( void )
{
	// We only call Reset() on previously freed contexts.  The command list persists, but we must
	// request a new allocator.
	/*
		ASSERT(m_CommandList != nullptr && m_CurrentAllocator == nullptr);
		m_CurrentAllocator = g_CommandManager.GetQueue(m_Type).RequestAllocator();
		m_CommandList->Reset(m_CurrentAllocator, nullptr);
		m_CurGraphicsRootSignature = nullptr;
		m_CurGraphicsPipelineState = nullptr;
		m_CurComputeRootSignature = nullptr;
		m_CurComputePipelineState = nullptr;
		m_NumBarriersToFlush = 0;

		BindDescriptorHeaps();
	*/
}

CommandContext::~CommandContext( void )
{
	if (m_CommandList != nullptr)
		m_CommandList->Release();
#ifdef GRAPHICS_DEBUG
	m_ConstantBufferAllocator.Destroy();
#endif
}

void CommandContext::Initialize( void )
{
    ComPtr<ID3D11DeviceContext> context;
	g_Device->CreateDeferredContext( 0, &context );

    ComPtr<ID3D11_CONTEXT> pContext;
    SUCCEEDED( context.As( &pContext) );
    m_CommandList = pContext.Detach();

	m_InternalCB.Create( L"InternalCB" );
	m_CpuLinearAllocator.Initialize( m_CommandList );
	m_GpuLinearAllocator.Initialize( m_CommandList );

#ifdef GRAPHICS_DEBUG
	m_ConstantBufferAllocator.Create();
#endif
}

static bool TimestampQueryInFlight = true;

void CommandContext::BeginQuery( ID3D11Query* pQueryDisjoint )
{
    if (!TimestampQueryInFlight) return;
    std::lock_guard<std::mutex> LockGuard( sm_ContextMutex );
    g_Context->Begin( pQueryDisjoint );
}

void CommandContext::EndQuery( ID3D11Query* pQueryDisjoint )
{
    if (!TimestampQueryInFlight) return;
    TimestampQueryInFlight = false;

    std::lock_guard<std::mutex> LockGuard( sm_ContextMutex );
    g_Context->End( pQueryDisjoint );
}

bool CommandContext::ResolveTimeStamps( ID3D11Query* pQueryDisjoint, ID3D11Query** pQueryHeap, uint32_t NumQueries, D3D11_QUERY_DATA_TIMESTAMP_DISJOINT* pDisjoint, uint64_t* pBuffer )
{
    std::lock_guard<std::mutex> LockGuard( sm_ContextMutex );

    if (S_OK != g_Context->GetData( pQueryDisjoint, pDisjoint, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), D3D11_ASYNC_GETDATA_DONOTFLUSH))
        return false;
    TimestampQueryInFlight = true;
    if (!pDisjoint->Disjoint)
    {
        for (uint32_t i = 0; i < NumQueries; i++)
        {
            if (S_OK != g_Context->GetData( pQueryHeap[i], &pBuffer[i], sizeof( UINT64 ), D3D11_ASYNC_GETDATA_DONOTFLUSH))
                pBuffer[i] = 0;
        }
    }
    return true;
}

void CommandContext::InsertTimeStamp( ID3D11Query* pQuery )
{
    if (!TimestampQueryInFlight) return;
    m_CommandList->End( pQuery );
}

void CommandContext::CopyBuffer( GpuResource& Dest, GpuResource& Src )
{
    m_CommandList->CopyResource( Dest.GetResource(), Src.GetResource() );
}

void CommandContext::CopySubresource( GpuResource& Dest, UINT DestSubIndex, GpuResource& Src, UINT SrcSubIndex )
{
    m_CommandList->CopySubresourceRegion( Dest.GetResource(), DestSubIndex, 0, 0, 0, Src.GetResource(), SrcSubIndex, nullptr );
}

void CommandContext::CopyCounter( GpuResource& Dest, size_t DestOffset, StructuredBuffer& Src)
{
    m_CommandList->CopyStructureCount( (ID3D11Buffer*)Dest.GetResource(), (UINT)DestOffset, Src.GetUAV() );
}

void ComputeContext::DispatchIndirect( GpuBuffer& ArgumentBuffer, size_t ArgumentBufferOffset )
{
    m_CommandList->DispatchIndirect( (ID3D11Buffer*)ArgumentBuffer.GetResource(), (UINT)ArgumentBufferOffset );
}

void CommandContext::SetDynamicDescriptor( UINT Offset, const D3D11_SRV_HANDLE Handle, BindList Binds )
{
	SetDynamicDescriptors( Offset, 1, &Handle, Binds );
}

void CommandContext::SetDynamicDescriptors( UINT Offset, UINT Count, const D3D11_SRV_HANDLE Handles[], BindList Binds )
{
	for (auto Bind : Binds )
	{
		switch (Bind)
		{
		case kBindVertex:		m_CommandList->VSSetShaderResources( Offset, Count, Handles ); break;
		case kBindHull:			m_CommandList->HSSetShaderResources( Offset, Count, Handles ); break;
		case kBindDomain:		m_CommandList->DSSetShaderResources( Offset, Count, Handles ); break;
		case kBindGeometry:		m_CommandList->GSSetShaderResources( Offset, Count, Handles ); break;
		case kBindPixel:		m_CommandList->PSSetShaderResources( Offset, Count, Handles ); break;
		case kBindCompute:		m_CommandList->CSSetShaderResources( Offset, Count, Handles ); break;
		}
	}
}

void ComputeContext::SetDynamicConstantBufferView( UINT Slot, size_t BufferSize, const void * BufferData )
{
    CommandContext::SetDynamicConstantBufferView( Slot, BufferSize, BufferData, { kBindCompute } );
}

void ComputeContext::SetDynamicDescriptor( UINT Offset, const D3D11_SRV_HANDLE Handle )
{
    m_CommandList->CSSetShaderResources( Offset, 1, &Handle );
}

void ComputeContext::SetDynamicDescriptor( UINT Offset, const D3D11_UAV_HANDLE Handle )
{
    m_CommandList->CSSetUnorderedAccessViews( Offset, 1, &Handle, nullptr );
}

void ComputeContext::SetDynamicDescriptors( UINT Offset, UINT Count, const D3D11_SRV_HANDLE Handles[] )
{
	m_CommandList->CSSetShaderResources( Offset, Count, Handles );
}

void ComputeContext::SetDynamicDescriptors( UINT Offset, UINT Count, const D3D11_UAV_HANDLE Handles[], const UINT *pUAVInitialCounts )
{
    m_CommandList->CSSetUnorderedAccessViews( Offset, Count, Handles, pUAVInitialCounts );
}

void CommandContext::SetDynamicSampler( UINT Offset, const D3D11_SAMPLER_HANDLE Handle,
	EPipelineBind Bind )
{
	SetDynamicSamplers( Offset, 1, &Handle, { Bind } );
}

void CommandContext::SetDynamicSamplers( UINT Offset, UINT Count,
	const D3D11_SAMPLER_HANDLE Handles[], BindList Binds )
{
	for (auto Bind : Binds )
	{
		switch (Bind)
		{
		case kBindVertex:		m_CommandList->VSSetSamplers( Offset, Count, Handles ); break;
		case kBindHull:			m_CommandList->HSSetSamplers( Offset, Count, Handles ); break;
		case kBindDomain:		m_CommandList->DSSetSamplers( Offset, Count, Handles ); break;
		case kBindGeometry:		m_CommandList->GSSetSamplers( Offset, Count, Handles ); break;
		case kBindPixel:		m_CommandList->PSSetSamplers( Offset, Count, Handles ); break;
		case kBindCompute:		m_CommandList->CSSetSamplers( Offset, Count, Handles ); break;
		}
	}
}


void ComputeContext::SetDynamicSampler( UINT Offset, const D3D11_SAMPLER_HANDLE Handle )
{
	m_CommandList->CSSetSamplers( Offset, 1, &Handle );
}

void ComputeContext::SetConstantBuffers( UINT Offset, UINT Count, const D3D11_BUFFER_HANDLE Handle[] )
{
	m_CommandList->CSSetConstantBuffers( Offset, Count, Handle );
}

void CommandContext::SetConstantBuffers( UINT Offset, UINT Count,
	const D3D11_BUFFER_HANDLE Handle[], BindList BindList )
{
	for (auto Bind : BindList)
	{
		switch (Bind)
		{
		case kBindVertex:		m_CommandList->VSSetConstantBuffers( Offset, Count, Handle ); break;
		case kBindHull:			m_CommandList->HSSetConstantBuffers( Offset, Count, Handle ); break;
		case kBindDomain:		m_CommandList->DSSetConstantBuffers( Offset, Count, Handle ); break;
		case kBindGeometry:		m_CommandList->GSSetConstantBuffers( Offset, Count, Handle ); break;
		case kBindPixel:		m_CommandList->PSSetConstantBuffers( Offset, Count, Handle ); break;
		case kBindCompute:		m_CommandList->CSSetConstantBuffers( Offset, Count, Handle ); break;
		}
	}
}

void CommandContext::UpdateBuffer( D3D11_BUFFER_HANDLE Handle, const void* Data, size_t Size )
{
	D3D11_MAPPED_SUBRESOURCE MapData = {};
	ASSERT_SUCCEEDED( m_CommandList->Map(
		Handle,
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&MapData ) );
	memcpy( MapData.pData, Data, Size );
	m_CommandList->Unmap( Handle, 0 );
}

void CommandContext::SetConstants( UINT Slot, UINT NumConstants, const void * pConstants, BindList BindList )
{
	m_InternalCB.Update(pConstants, sizeof(UINT) * NumConstants );
	m_InternalCB.UploadAndBind( *this, Slot, BindList );
}

void CommandContext::SetConstants( UINT Slot, DWParam X, BindList BindList )
{
	SetConstants( Slot, 1, &X, BindList );
}

void CommandContext::SetConstants( UINT Slot, DWParam X, DWParam Y, BindList BindList )
{
	DWParam Param[] = { X, Y };
	SetConstants( Slot, 2, &Param, BindList );
}

void CommandContext::SetConstants( UINT Slot, DWParam X, DWParam Y, DWParam Z, BindList BindList )
{
	DWParam Param[] = { X, Y, Z };
	SetConstants( Slot, 3, &Param, BindList );
}

void CommandContext::SetConstants( UINT Slot, DWParam X, DWParam Y, DWParam Z, DWParam W, BindList BindList )
{
	DWParam Param[] = { X, Y, Z, W };
	SetConstants( Slot, 4, &Param, BindList );
}

void ComputeContext::SetConstants( UINT Slot, UINT NumConstants, const void* pConstants )
{
    CommandContext::SetConstants( Slot, NumConstants, pConstants, { kBindCompute } );
}

void ComputeContext::SetConstants( UINT Slot, DWParam X )
{
    CommandContext::SetConstants( Slot, X, { kBindCompute } );
}

void ComputeContext::SetConstants( UINT Slot, DWParam X, DWParam Y )
{
    CommandContext::SetConstants( Slot, X, Y, { kBindCompute } );
}

void ComputeContext::SetConstants( UINT Slot, DWParam X, DWParam Y, DWParam Z )
{
    CommandContext::SetConstants( Slot, X, Y, Z, { kBindCompute } );
}

void ComputeContext::SetConstants( UINT Slot, DWParam X, DWParam Y, DWParam Z, DWParam W )
{
    CommandContext::SetConstants( Slot, X, Y, Z, W, { kBindCompute } );
}

#ifndef GRAPHICS_DEBUG
void CommandContext::SetDynamicConstantBufferView( UINT Slot, size_t BufferSize, const void* BufferData, BindList Binds )
{
	ASSERT( BufferData != nullptr && Math::IsAligned( BufferData, 16 ) );

	DynAlloc Alloc = m_CpuLinearAllocator.Allocate( BufferSize );
	ASSERT(Alloc.DataPtr != nullptr);

	memcpy(Alloc.DataPtr, BufferData, BufferSize );

	ID3D11Buffer* Buffers[] = { Alloc.Handle };
	for (auto Bind : Binds )
	{
		switch (Bind)
		{
		case kBindVertex:		m_CommandList->VSSetConstantBuffers1( Slot, 1, Buffers, &Alloc.FirstConstant, &Alloc.NumConstants ); break;
		case kBindHull:			m_CommandList->HSSetConstantBuffers1( Slot, 1, Buffers, &Alloc.FirstConstant, &Alloc.NumConstants ); break;
		case kBindDomain:		m_CommandList->DSSetConstantBuffers1( Slot, 1, Buffers, &Alloc.FirstConstant, &Alloc.NumConstants ); break;
		case kBindGeometry:		m_CommandList->GSSetConstantBuffers1( Slot, 1, Buffers, &Alloc.FirstConstant, &Alloc.NumConstants ); break;
		case kBindPixel:		m_CommandList->PSSetConstantBuffers1( Slot, 1, Buffers, &Alloc.FirstConstant, &Alloc.NumConstants ); break;
		case kBindCompute:		m_CommandList->CSSetConstantBuffers1( Slot, 1, Buffers, &Alloc.FirstConstant, &Alloc.NumConstants ); break;
		}
	}
}
#else
void CommandContext::SetDynamicConstantBufferView( UINT Slot, size_t BufferSize, const void* BufferData, BindList Binds )
{
	ASSERT( BufferData != nullptr && Math::IsAligned( BufferData, 16 ) );

	std::vector<EPipelineBind> bind(Binds);
	auto& Page = m_ConstantBufferAllocator.m_PagePool[Slot + bind.front() * D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	Page->Map( m_CommandList );
	ASSERT(Page->m_CpuVirtualAddress != nullptr);
	memcpy(Page->m_CpuVirtualAddress, BufferData, BufferSize );
	Page->Unmap( m_CommandList );
	auto Handle = reinterpret_cast<ID3D11Buffer*>(Page->GetResource());

	ID3D11Buffer* Buffers[] = { Handle };
	for (auto Bind : Binds )
	{
		switch (Bind)
		{
		case kBindVertex:		m_CommandList->VSSetConstantBuffers( Slot, 1, Buffers ); break;
		case kBindHull:			m_CommandList->HSSetConstantBuffers( Slot, 1, Buffers ); break;
		case kBindDomain:		m_CommandList->DSSetConstantBuffers( Slot, 1, Buffers ); break;
		case kBindGeometry:		m_CommandList->GSSetConstantBuffers( Slot, 1, Buffers ); break;
		case kBindPixel:		m_CommandList->PSSetConstantBuffers( Slot, 1, Buffers ); break;
		case kBindCompute:		m_CommandList->CSSetConstantBuffers( Slot, 1, Buffers ); break;
		}
	}
}
#endif

void GraphicsContext::SetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY Topology )
{
	m_CommandList->IASetPrimitiveTopology( Topology );
}

void GraphicsContext::SetRenderTargets( UINT NumRTVs, const D3D11_RTV_HANDLE RTVs[] )
{
	m_CommandList->OMSetRenderTargets( NumRTVs, RTVs, nullptr);
}

void GraphicsContext::SetRenderTargets( UINT NumRTVs, const D3D11_RTV_HANDLE RTVs[], D3D11_DSV_HANDLE DSV )
{
	m_CommandList->OMSetRenderTargets( NumRTVs, RTVs, DSV );
}

void GraphicsContext::SetRenderTarget( D3D11_RTV_HANDLE RTV, D3D11_DSV_HANDLE DSV ) {
	SetRenderTargets( 1, &RTV, DSV );
}

void GraphicsContext::ClearColor( ColorBuffer & Target )
{
	m_CommandList->ClearRenderTargetView( Target.GetRTV(), Target.GetClearColor().GetPtr() );
}

void GraphicsContext::ClearDepth( DepthBuffer& Target )
{
    ASSERT( Target.GetDSV() != nullptr );
	m_CommandList->ClearDepthStencilView( Target.GetDSV(), D3D11_CLEAR_DEPTH, Target.GetClearDepth(), Target.GetClearStencil() );
}

void GraphicsContext::ClearDepth( DepthBuffer& Target, uint32_t Slice )
{
    ASSERT( Target.GetDSV(Slice) != nullptr );
	m_CommandList->ClearDepthStencilView( Target.GetDSV(Slice), D3D11_CLEAR_DEPTH, Target.GetClearDepth(), Target.GetClearStencil() );
}

void GraphicsContext::ClearStencil( DepthBuffer& Target )
{
	m_CommandList->ClearDepthStencilView( Target.GetDSV(), D3D11_CLEAR_STENCIL, Target.GetClearDepth(), Target.GetClearStencil() );
}

void GraphicsContext::ClearDepthAndStencil( DepthBuffer& Target )
{
	m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, Target.GetClearDepth(), Target.GetClearStencil() );
}

void GraphicsContext::SetViewportAndScissor( const D3D11_VIEWPORT& vp, const D3D11_RECT& rect )
{
	ASSERT(rect.left < rect.right && rect.top < rect.bottom);
	m_CommandList->RSSetViewports( 1, &vp );
	m_CommandList->RSSetScissorRects( 1, &rect );
}

void GraphicsContext::GenerateMips( D3D11_SRV_HANDLE SRV )
{
	m_CommandList->GenerateMips( SRV );
}

void GraphicsContext::SetViewport( const D3D11_VIEWPORT& vp )
{
	m_CommandList->RSSetViewports( 1, &vp );
}

void GraphicsContext::SetViewport( FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth, FLOAT maxDepth )
{
	D3D11_VIEWPORT vp;
	vp.Width = w;
	vp.Height = h;
	vp.MinDepth = minDepth;
	vp.MaxDepth = maxDepth;
	vp.TopLeftX = x;
	vp.TopLeftY = y;
	m_CommandList->RSSetViewports( 1, &vp );
}

void GraphicsContext::SetScissor( const D3D11_RECT& rect )
{
	ASSERT(rect.left < rect.right && rect.top < rect.bottom);
	m_CommandList->RSSetScissorRects( 1, &rect );
}

void GraphicsContext::SetIndexBuffer( const D3D11_INDEX_BUFFER_VIEW& Buffer )
{
	m_CommandList->IASetIndexBuffer( Buffer.Buffer, Buffer.Format, Buffer.Offset );
}

void GraphicsContext::SetVertexBuffer( UINT Slot, const D3D11_VERTEX_BUFFER_VIEW& Buffer )
{
	m_CommandList->IASetVertexBuffers( Slot, 1, &Buffer.Buffer, &Buffer.StrideInBytes, &Buffer.Offset );
}

void GraphicsContext::SetDynamicVB( UINT Slot, size_t NumVertices, size_t VertexStride, const void* VBData )
{
    VertexBuffer buffer;
    buffer.Create( L"DynamicVB", (uint32_t)NumVertices, (uint32_t)VertexStride, VBData );
    SetVertexBuffer( Slot, buffer.VertexBufferView() );
}

void ComputeContext::SetPipelineState( ComputePSO& PSO )
{
	m_PSOState = PSO.GetState();
	m_PSOState->Bind( m_CommandList );
}

void GraphicsContext::SetPipelineState( GraphicsPSO& PSO )
{
	m_PSOState = PSO.GetState();
	m_PSOState->Bind( m_CommandList );
}

#ifdef GRAPHICS_DEBUG
LinearAllocatorPageManager ConstantBufferAllocator::sm_PageManager = kCpuWritable;

void ConstantBufferAllocator::Create()
{
    static_assert(kBindCompute+1 == 6, "test if total length");
	for (int i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT*6; i++)
		m_PagePool.emplace_back( sm_PageManager.CreateNewPage( 0x10000 ) );
}

void ConstantBufferAllocator::Destroy()
{
	m_PagePool.clear();
}
#endif

