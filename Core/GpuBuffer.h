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
#include "Mapping.h"
#include "GpuResource.h"

class CommandContext;
class EsramAllocator;
struct D3D11_VERTEX_BUFFER_VIEW;
struct D3D11_INDEX_BUFFER_VIEW;

class GpuBuffer : public GpuResource
{
public:
	virtual ~GpuBuffer() { Destroy(); }

	virtual void Destroy( void );

	// Create a buffer.  If initial data is provided, it will be copied into the buffer using the default command context.
	void Create( const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
		const void* initialData = nullptr );

	// Create a buffer in ESRAM.  On Windows, ESRAM is not used.
	void Create( const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
		EsramAllocator& Allocator, const void* initialData = nullptr);

	const D3D11_UAV_HANDLE GetUAV(void) const { return m_UAV.Get(); }
	const D3D11_SRV_HANDLE GetSRV(void) const { return m_SRV.Get(); }

	void SetBindFlag( UINT Flags ) { m_BindFlags |= Flags; }
    void UnsetBindFlag( UINT Flags ) { m_BindFlags &= ~Flags; }
	void SetUsage( D3D11_USAGE Usage ) { m_Usage = Usage; }
	void SetCPUAccess( D3D11_CPU_ACCESS_FLAG Access ) { m_CPUAccessFlags |= Access; }

	D3D11_VERTEX_BUFFER_VIEW VertexBufferView( uint32_t Offset, uint32_t Stride ) const;
	D3D11_VERTEX_BUFFER_VIEW VertexBufferView( uint32_t BaseVertexIndex = 0 ) const;

	D3D11_INDEX_BUFFER_VIEW IndexBufferView( uint32_t Offset, bool b32Bit = false ) const;
	D3D11_INDEX_BUFFER_VIEW IndexBufferView( uint32_t StartIndex = 0 ) const;

    D3D11_BUFFER_HANDLE GetHandle(void) { return m_Buffer.Get(); }

protected:

	GpuBuffer( void );

	virtual void CreateDerivedViews( void ) = 0;

	size_t m_BufferSize;
	uint32_t m_ElementCount;
	uint32_t m_ElementSize;
	UINT m_CPUAccessFlags;
	UINT m_BindFlags;
	UINT m_MiscFlags;
	D3D11_USAGE m_Usage;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_SRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_UAV;
};

class ByteAddressBuffer : public GpuBuffer
{
public:
    ByteAddressBuffer( void );
	virtual void CreateDerivedViews( void ) override;
};

class IndirectArgsBuffer : public ByteAddressBuffer
{
public:
    IndirectArgsBuffer( void );
};

class StructuredBuffer : public GpuBuffer
{
public:
	StructuredBuffer(bool bUseCounter = true);
	virtual void Destroy( void ) override;

	virtual void CreateDerivedViews( void ) override;

	ByteAddressBuffer& GetCounterBuffer( void ) { return m_CounterBuffer; }

    void FillCounter(CommandContext& Context);
	const D3D11_SRV_HANDLE GetCounterSRV();
	const D3D11_UAV_HANDLE GetCounterUAV();

private:
    bool m_bUseCounter;
	ByteAddressBuffer m_CounterBuffer;
};


class IndexBuffer : public ByteAddressBuffer
{
public:
	IndexBuffer();
};

class VertexBuffer : public GpuBuffer
{
public:
	VertexBuffer();
	virtual void CreateDerivedViews( void ) override {}
};

class TypedBuffer : public GpuBuffer
{
public:
    TypedBuffer( DXGI_FORMAT Format );
    virtual void CreateDerivedViews(void) override;

protected:
    DXGI_FORMAT m_DataFormat;
};