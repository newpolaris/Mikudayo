#pragma once

#include "PixelBuffer.h"
#include "Mapping.h"

class DepthBuffer : public PixelBuffer
{
public:
	DepthBuffer( float ClearDepth = 0.0f, uint8_t ClearStencil = 0 )
		: m_ClearDepth(ClearDepth), m_ClearStencil(ClearStencil), m_FragmentCount(1) {}

	// Create a depth buffer.
	void Create( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format );

	// Create a depth buffer.  Memory will be allocated in ESRAM (on Xbox One).  On Windows,
	// this functions the same as Create() without a video address.
	void Create( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format,
		EsramAllocator& Allocator );

	void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format );

	void Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format,
		EsramAllocator& Allocator );

	// Create a depth buffer
	void CreateArray( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
		DXGI_FORMAT Format );

	// Create a depth buffer.  Memory will be allocated in ESRAM (on Xbox One).  On Windows,
	// this functions the same as Create() without a video address.
	void CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
		DXGI_FORMAT Format, EsramAllocator& Allocator);

	// Get pre-created CPU-visible descriptor handles
	const D3D11_DSV_HANDLE& GetDSV() const { return m_hDSV[0]; }
	const D3D11_DSV_HANDLE& GetDSV_DepthReadOnly() const { return m_hDSV[1]; }
	const D3D11_DSV_HANDLE& GetDSV_StencilReadOnly() const { return m_hDSV[2]; }
	const D3D11_DSV_HANDLE& GetDSV_ReadOnly() const { return m_hDSV[3]; }
    const D3D11_DSV_HANDLE& GetDSVSlice( uint32_t slice ) const { return m_hDSVSlice[slice]; }
	const D3D11_SRV_HANDLE  GetDepthSRV() const { return m_hDepthSRV.Get(); }
	const D3D11_SRV_HANDLE  GetStencilSRV() const { return m_hStencilSRV.Get(); }

	float GetClearDepth() const { return m_ClearDepth; }
	uint8_t GetClearStencil() const { return m_ClearStencil; }

	void SetClearDepth( float ClearDeapth ) { m_ClearDepth = ClearDeapth; }
	void SetClearStencil( uint8_t ClearStencil ) { m_ClearStencil = ClearStencil; }

	void Destroy();

private:

	void CreateDerivedViews( ID3D11Device* Device, DXGI_FORMAT Format, uint32_t ArraySize );

	float m_ClearDepth;
	uint8_t m_ClearStencil;
	uint32_t m_FragmentCount;

	static const uint32_t kDSVSize = 4;
	ID3D11DepthStencilView* m_hDSV[kDSVSize];
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_hDSVHandle[kDSVSize];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_hDepthSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_hStencilSRV;

    std::vector<D3D11_DSV_HANDLE> m_hDSVSlice;
    std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>> m_hDSVSliceHandle;
};
