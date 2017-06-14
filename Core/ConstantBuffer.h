#pragma once

#include "pch.h"
#include "CommandContext.h"
#include "GraphicsCore.h"

template <typename T>
class ConstantBuffer
{
public:
    ConstantBuffer()
    {
	}

	ConstantBuffer( const ConstantBuffer& rhs ) = delete;
	ConstantBuffer& operator=( const ConstantBuffer& rhs ) = delete;
	~ConstantBuffer() {}

	void Create( std::wstring Name = L"ConstantBuffer") 
	{
		D3D11_BUFFER_DESC Desc;
		Desc.ByteWidth = sizeof(T);
		Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		Desc.MiscFlags = 0;
		Desc.StructureByteStride = 0;
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer;
		ASSERT_SUCCEEDED( g_Device->CreateBuffer( &Desc, nullptr, Buffer.GetAddressOf() ) );
		m_Buffer.Swap( Buffer );

		SetName( m_Buffer, Name );
	}

	void Destory()
	{
		m_Buffer = nullptr;
	}

	ID3D11Buffer* Resource() const
	{
		return static_cast<ID3D11Buffer*>(mUploadBuffer[elementIndex].Get());
	}

	void Update( T CpuLocal )
	{
		m_CpuLocal = std::move( CpuLocal );
	}

	void Update( const void* pData, size_t Size )
	{
		ASSERT( Size <= sizeof(UINT) * CommandContext::kNumConstant );
		memcpy( &m_CpuLocal, pData, Size);
	}

	void UploadAndBind( CommandContext& Context, UINT Slot, BindList BindList ) const
	{
		Context.UploadContstantBuffer( m_Buffer.Get(), &m_CpuLocal, sizeof(m_CpuLocal) );

		ID3D11Buffer* Buffers[] = { m_Buffer.Get() };
		Context.SetConstantBuffers( Slot, 1, Buffers, BindList );
	}

private:
	T m_CpuLocal;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer;
};

