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

	return ret;
}

void ContextManager::FreeContext(CommandContext* UsedContext)
{
	ASSERT(UsedContext != nullptr);
	std::lock_guard<std::mutex> LockGuard( sm_ContextAllocationMutex );
	sm_AvailableContexts[UsedContext->m_Type].push(UsedContext);
}

void CommandContext::DestroyAllContexts(void)
{
	g_ContextManager.DestroyAllContexts();
}

CommandContext& CommandContext::Begin( ContextType Type, const std::wstring ID )
{
	CommandContext* NewContext = g_ContextManager.AllocateContext( Type );
	NewContext->SetID( ID );
	// if (ID.length() > 0)
	//	EngineProfiling::BeginBlock(ID, NewContext);

	return *NewContext;
}

uint64_t CommandContext::Flush(bool WaitForCompletion)
{
	m_CpuLinearAllocator.Finish();
	ComPtr<ID3D11CommandList> CommandList;
	m_Context->FinishCommandList( FALSE, &CommandList );
	g_Context->ExecuteCommandList( CommandList.Get(), FALSE );

	return 0;
}

uint64_t CommandContext::Finish( bool WaitForCompletion )
{
	// if (m_ID.length() > 0)
	//	EngineProfiling::EndBlock(this);

	Flush( WaitForCompletion );
	g_ContextManager.FreeContext( this );

	return 0;
}

CommandContext::CommandContext()
{
	m_OwningManager = nullptr;
	m_Context = nullptr;
}

void CommandContext::Reset( void )
{
	// We only call Reset() on previously freed contexts.  The command list persists, but we must
	// request a new allocator.
	/*
	ASSERT(m_CommandList != nullptr && m_CurrentAllocator == nullptr);
	m_CurrentAllocator = g_CommandManager.GetQueue(m_Type).RequestAllocator();
	m_CommandList->Reset(m_CurrentAllocator, nullptr);
m_CommandList->

	m_CurGraphicsRootSignature = nullptr;
	m_CurGraphicsPipelineState = nullptr;
	m_CurComputeRootSignature = nullptr;
	m_CurComputePipelineState = nullptr;
	m_NumBarriersToFlush = 0;

	BindDescriptorHeaps();
}*/
	m_CpuLinearAllocator.Reset();
}

CommandContext::~CommandContext( void )
{
	if (m_Context != nullptr)
		m_Context->Release();
}

void CommandContext::Initialize( void )
{
	g_Device->CreateDeferredContext3( 0, &m_Context );

	m_InternalCB.Create( L"InternalCB" );
	m_CpuLinearAllocator.Initialize( m_Context );
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
		case kBindVertex:		m_Context->VSSetShaderResources( Offset, Count, Handles ); break;
		case kBindHull:			m_Context->HSSetShaderResources( Offset, Count, Handles ); break;
		case kBindDomain:		m_Context->DSSetShaderResources( Offset, Count, Handles ); break;
		case kBindGeometry:		m_Context->GSSetShaderResources( Offset, Count, Handles ); break;
		case kBindPixel:		m_Context->PSSetShaderResources( Offset, Count, Handles ); break;
		case kBindCompute:		m_Context->CSSetShaderResources( Offset, Count, Handles ); break;
		}
	}
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
		case kBindVertex:		m_Context->VSSetSamplers( Offset, Count, Handles ); break;
		case kBindHull:			m_Context->HSSetSamplers( Offset, Count, Handles ); break;
		case kBindDomain:		m_Context->DSSetSamplers( Offset, Count, Handles ); break;
		case kBindGeometry:		m_Context->GSSetSamplers( Offset, Count, Handles ); break;
		case kBindPixel:		m_Context->PSSetSamplers( Offset, Count, Handles ); break;
		case kBindCompute:		m_Context->CSSetSamplers( Offset, Count, Handles ); break;
		}
	}
}

void CommandContext::SetConstantBuffers( UINT Offset, UINT Count,
	const D3D11_BUFFER_HANDLE Handle[], BindList BindList )
{
	for (auto Bind : BindList)
	{
		switch (Bind)
		{
		case kBindVertex:		m_Context->VSSetConstantBuffers( Offset, Count, Handle ); break;
		case kBindHull:			m_Context->HSSetConstantBuffers( Offset, Count, Handle ); break;
		case kBindDomain:		m_Context->DSSetConstantBuffers( Offset, Count, Handle ); break;
		case kBindGeometry:		m_Context->GSSetConstantBuffers( Offset, Count, Handle ); break;
		case kBindPixel:		m_Context->PSSetConstantBuffers( Offset, Count, Handle ); break;
		case kBindCompute:		m_Context->CSSetConstantBuffers( Offset, Count, Handle ); break;
		}
	}
}

void CommandContext::UploadContstantBuffer( D3D11_BUFFER_HANDLE Handle, const void* Data, size_t Size )
{
	D3D11_MAPPED_SUBRESOURCE MapData = {};
	ASSERT_SUCCEEDED( m_Context->Map(
		Handle,
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&MapData ) );
	memcpy( MapData.pData, Data, Size );
	m_Context->Unmap( Handle, 0 );
}

void CommandContext::SetConstants( UINT NumConstants, const void * pConstants, BindList BindList )
{
	m_InternalCB.Update(pConstants, sizeof(UINT) * NumConstants );
	m_InternalCB.UploadAndBind( *this, 0, BindList );
}

void CommandContext::SetConstants( DWParam X, BindList BindList )
{
	SetConstants( 1, &X, BindList );
}

void CommandContext::SetConstants( DWParam X, DWParam Y, BindList BindList )
{
	DWParam Param[] = { X, Y };
	SetConstants( 2, &Param, BindList );
}

void CommandContext::SetConstants( DWParam X, DWParam Y, DWParam Z, BindList BindList )
{
	DWParam Param[] = { X, Y, Z };
	SetConstants( 3, &Param, BindList );
}

void CommandContext::SetConstants( DWParam X, DWParam Y, DWParam Z, DWParam W, BindList BindList )
{
	DWParam Param[] = { X, Y, Z, W };
	SetConstants( 4, &Param, BindList );
}

GraphicsContext::GraphicsContext()
{
	m_Type = kGraphicsContext;
}

DynAlloc LinearAllocator::Allocate( size_t SizeInBytes, size_t Alignment )
{
	if (!m_Buffer)
		CreatePage();

	const size_t AlignmentMask = Alignment - 1;

	// Assert that it's a power of two.
	ASSERT( (AlignmentMask & Alignment) == 0 );

	// Align the allocation
	const size_t AlignedSize = Math::AlignUpWithMask(SizeInBytes, AlignmentMask);

	ASSERT( AlignedSize <= m_PageSize );

	m_CurOffset = Math::AlignUp( m_CurOffset, Alignment );

	ASSERT( m_CurOffset + AlignedSize <= m_PageSize );

	DynAlloc ret( m_Buffer.Get(),
		static_cast<UINT>(m_CurOffset / 16),
		static_cast<UINT>(AlignedSize / 16) );
	ret.DataPtr = m_AllocPointer;

	m_AllocPointer += AlignedSize;
	m_CurOffset += AlignedSize;

	return ret;
}

void LinearAllocator::CreatePage()
{
	D3D11_BUFFER_DESC Desc;
	Desc.ByteWidth = static_cast<UINT>(InitSize);
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.MiscFlags = 0;
	Desc.StructureByteStride = 0;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer;
	ASSERT_SUCCEEDED( g_Device->CreateBuffer( &Desc, nullptr, Buffer.GetAddressOf() ) );
	m_Buffer.Swap( Buffer );
	SetName( m_Buffer, L"LinearBuffer" );

	D3D11_MAPPED_SUBRESOURCE MapData = {};
	ASSERT_SUCCEEDED( m_Context->Map(
		m_Buffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&MapData ) );

	m_CurOffset = 0;
	m_AllocPointer = reinterpret_cast<uint8_t*>(MapData.pData);
}

void LinearAllocator::Reset( )
{
	if (!m_Buffer)
		return;

	D3D11_MAPPED_SUBRESOURCE MapData = {};
	ASSERT_SUCCEEDED( m_Context->Map(
		m_Buffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&MapData ) );

	m_CurOffset = 0;
	m_AllocPointer = reinterpret_cast<uint8_t*>(MapData.pData);
}

void CommandContext::SetDynamicConstantBufferView( UINT Slot, size_t BufferSize, const void* BufferData, BindList Binds )
{
	ASSERT( BufferData != nullptr && Math::IsAligned( BufferData, 16 ) );

	DynAlloc Alloc = m_CpuLinearAllocator.Allocate( BufferSize );

	memcpy(Alloc.DataPtr, BufferData, BufferSize );

	ID3D11Buffer* Buffers[] = { Alloc.Handle };
	for (auto Bind : Binds )
	{
		switch (Bind)
		{
		case kBindVertex:		m_Context->VSSetConstantBuffers1( Slot, 1, Buffers, &Alloc.FirstConstant, &Alloc.NumConstants ); break;
		case kBindHull:			m_Context->HSSetConstantBuffers1( Slot, 1, Buffers, &Alloc.FirstConstant, &Alloc.NumConstants ); break;
		case kBindDomain:		m_Context->DSSetConstantBuffers1( Slot, 1, Buffers, &Alloc.FirstConstant, &Alloc.NumConstants ); break;
		case kBindGeometry:		m_Context->GSSetConstantBuffers1( Slot, 1, Buffers, &Alloc.FirstConstant, &Alloc.NumConstants ); break;
		case kBindPixel:		m_Context->PSSetConstantBuffers1( Slot, 1, Buffers, &Alloc.FirstConstant, &Alloc.NumConstants ); break;
		case kBindCompute:		m_Context->CSSetConstantBuffers1( Slot, 1, Buffers, &Alloc.FirstConstant, &Alloc.NumConstants ); break;
		}
	}
}

void GraphicsContext::SetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY Topology )
{
	m_Context->IASetPrimitiveTopology( Topology );
}

void GraphicsContext::SetRenderTargets( UINT NumRTVs, const D3D11_RTV_HANDLE RTVs[] )
{
	m_Context->OMSetRenderTargets( NumRTVs, RTVs, nullptr);
}

void GraphicsContext::SetRenderTargets( UINT NumRTVs, const D3D11_RTV_HANDLE RTVs[], D3D11_DSV_HANDLE DSV )
{
	m_Context->OMSetRenderTargets( NumRTVs, RTVs, DSV );
}

void GraphicsContext::SetRenderTarget( D3D11_RTV_HANDLE RTV, D3D11_DSV_HANDLE DSV ) { 
	SetRenderTargets( 1, &RTV, DSV ); 
}

void GraphicsContext::ClearColor( ColorBuffer & Target )
{
	m_Context->ClearRenderTargetView( Target.GetRTV(), Target.GetClearColor().GetPtr() );
}

void GraphicsContext::ClearDepth( DepthBuffer& Target )
{
	m_Context->ClearDepthStencilView( Target.GetDSV(), D3D11_CLEAR_DEPTH, Target.GetClearDepth(), Target.GetClearStencil() );
}

void GraphicsContext::ClearStencil( DepthBuffer& Target )
{
	m_Context->ClearDepthStencilView( Target.GetDSV(), D3D11_CLEAR_STENCIL, Target.GetClearDepth(), Target.GetClearStencil() );
}

void GraphicsContext::ClearDepthAndStencil( DepthBuffer& Target )
{
	m_Context->ClearDepthStencilView(Target.GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, Target.GetClearDepth(), Target.GetClearStencil() );
}

void GraphicsContext::SetViewportAndScissor( const D3D11_VIEWPORT& vp, const D3D11_RECT& rect )
{
	ASSERT(rect.left < rect.right && rect.top < rect.bottom);
	m_Context->RSSetViewports( 1, &vp );
	m_Context->RSSetScissorRects( 1, &rect );
}

void GraphicsContext::GenerateMips( D3D11_SRV_HANDLE SRV )
{
	m_Context->GenerateMips( SRV );
}

void GraphicsContext::SetViewport( const D3D11_VIEWPORT& vp )
{
	m_Context->RSSetViewports( 1, &vp );
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
	m_Context->RSSetViewports( 1, &vp );
}

void GraphicsContext::SetScissor( const D3D11_RECT& rect )
{
	ASSERT(rect.left < rect.right && rect.top < rect.bottom);
	m_Context->RSSetScissorRects( 1, &rect );
}

void GraphicsContext::SetIndexBuffer( const D3D11_INDEX_BUFFER_VIEW& Buffer )
{
	m_Context->IASetIndexBuffer( Buffer.Buffer, Buffer.Format, Buffer.Offset );
}

void GraphicsContext::SetVertexBuffer( UINT Slot, const D3D11_VERTEX_BUFFER_VIEW& Buffer )
{
	m_Context->IASetVertexBuffers( Slot, 1, &Buffer.Buffer, &Buffer.StrideInBytes, &Buffer.Offset );
}

void GraphicsContext::SetPipelineState( GraphicsPSO& PSO )
{
	m_PSOState = PSO.GetState();
	m_PSOState->Bind( m_Context );
}

