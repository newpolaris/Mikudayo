#include "pch.h"
#include "GpuBuffer.h"
#include "GraphicsCore.h"
#include "CommandContext.h"
// #include "BufferManager.h"

using namespace Graphics;

D3D11_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView( uint32_t Offset, uint32_t Stride ) const
{
	D3D11_VERTEX_BUFFER_VIEW VBView;
	VBView.Buffer = m_Buffer.Get();
	VBView.StrideInBytes = Stride;
	VBView.Offset = Offset;
	return VBView;
}

D3D11_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView( uint32_t BaseVertexIndex ) const
{
	uint32_t Offset = BaseVertexIndex * m_ElementSize;
	return VertexBufferView( Offset, m_ElementSize );
}

D3D11_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView( uint32_t Offset, bool b32Bit ) const
{
	D3D11_INDEX_BUFFER_VIEW IBView;
	IBView.Buffer = m_Buffer.Get();
	IBView.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
	IBView.Offset = Offset;
	return IBView;
}

D3D11_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView( uint32_t StartIndex ) const
{
	uint32_t Offset = StartIndex * m_ElementSize;
	return IndexBufferView( Offset, m_ElementSize == 4 );
}

GpuBuffer::GpuBuffer( void ) :
	m_BufferSize( 0 ), m_ElementCount( 0 ), m_ElementSize( 0 ),
	m_BindFlags( 0 ), m_CPUAccessFlags( 0 ), m_MiscFlags( 0 ),
	m_Usage( D3D11_USAGE_DEFAULT )
{
}

void GpuBuffer::Create( const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, const void* initialData )
{
	m_ElementCount = NumElements;
	m_ElementSize = ElementSize;
	m_BufferSize = NumElements * ElementSize;

	ASSERT(!(m_Usage == D3D11_USAGE_IMMUTABLE && m_BindFlags & D3D11_BIND_UNORDERED_ACCESS));

	D3D11_BUFFER_DESC Desc;
	Desc.ByteWidth = static_cast<UINT>(m_BufferSize);
	Desc.BindFlags = m_BindFlags;
	Desc.Usage = m_Usage;
	Desc.CPUAccessFlags = m_CPUAccessFlags;
	Desc.MiscFlags = m_MiscFlags;
	Desc.StructureByteStride = 0;
	if (m_MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
		Desc.StructureByteStride = m_ElementSize;

	D3D11_SUBRESOURCE_DATA Data;
	Data.pSysMem = initialData;
	Data.SysMemPitch = 0;
	Data.SysMemSlicePitch = 0;

	D3D11_SUBRESOURCE_DATA* pData = (initialData == nullptr) ? nullptr : &Data;

	ASSERT_SUCCEEDED( g_Device->CreateBuffer( &Desc, pData, m_Buffer.ReleaseAndGetAddressOf() ) );
	m_Buffer.As(&m_pResource);

	SetName( m_pResource, name );

	CreateDerivedViews();
}

void GpuBuffer::Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
	EsramAllocator&, const void* initialData)
{
	Create(name, NumElements, ElementSize, initialData);
}

void GpuBuffer::Destroy( void )
{
	m_Buffer = nullptr;
	m_SRV = nullptr;
	GpuResource::Destroy();
}

void StructuredBuffer::Destroy( void )
{
	m_CounterBuffer.Destroy();
	GpuBuffer::Destroy();
}

void ByteAddressBuffer::CreateDerivedViews( void )
{
	if (m_BindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		SRVDesc.BufferEx.FirstElement = 0;
		SRVDesc.BufferEx.NumElements = (UINT)m_BufferSize / 4;
		SRVDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;

		ASSERT_SUCCEEDED( g_Device->CreateShaderResourceView( m_pResource.Get(), &SRVDesc, &m_SRV ) );
	}

	if (m_BindFlags & D3D11_BIND_UNORDERED_ACCESS)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		UAVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
		UAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

		ASSERT_SUCCEEDED( g_Device->CreateUnorderedAccessView( m_pResource.Get(), &UAVDesc, &m_UAV ) );
	}
}

IndexBuffer::IndexBuffer()
{
	m_Usage = D3D11_USAGE_IMMUTABLE;

	m_BindFlags |= D3D11_BIND_INDEX_BUFFER | D3D11_BIND_SHADER_RESOURCE;
	m_MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
}

VertexBuffer::VertexBuffer()
{
	m_Usage = D3D11_USAGE_IMMUTABLE;
	m_BindFlags = D3D11_BIND_VERTEX_BUFFER;
}

StructuredBuffer::StructuredBuffer( void ) 
{
	m_Usage = D3D11_USAGE_DEFAULT;

	m_BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	m_MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
}

void StructuredBuffer::CreateDerivedViews( void ) 
{
	if (m_BindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		SRVDesc.BufferEx.NumElements = m_ElementCount;
		SRVDesc.BufferEx.Flags = 0;

		ASSERT_SUCCEEDED( g_Device->CreateShaderResourceView( m_pResource.Get(), &SRVDesc, &m_SRV ) );
	}
}
