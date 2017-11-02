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

#include "PixelBuffer.h"
#include "Color.h"
#include "EsramAllocator.h"
#include "Mapping.h"
#include "IColorBuffer.h"

class ColorBuffer : public PixelBuffer, public IColorBuffer
{
public:
	ColorBuffer( Color ClearColor = Color(0.0f, 0.0f, 0.0f, 0.0f) )
		: m_ClearColor(ClearColor), m_NumMipMaps(0), m_FragmentCount(1), m_SampleCount(1)
	{
	}

	// Create a color buffer from a swap chain buffer.  Unordered access is restricted.
	void CreateFromSwapChain( const std::wstring& Name, Microsoft::WRL::ComPtr<ID3D11Texture2D> BaseResource );

	// Create a color buffer
	void Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
		DXGI_FORMAT Format );

	void Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
		DXGI_FORMAT Format, EsramAllocator& );

	// Create a color buffer
	void CreateArray( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
		DXGI_FORMAT Format );

	// Create a color buffer.  Memory will be allocated in ESRAM (on Xbox One).  On Windows,
	// this functions the same as Create() without a video address.
	void CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
		DXGI_FORMAT Format, EsramAllocator& Allocator);

	// Get pre-created CPU-visible descriptor handles
	const D3D11_SRV_HANDLE GetSRV( void ) const { return m_SRVHandle.Get(); }
	const D3D11_RTV_HANDLE GetRTV( void ) const { return m_RTVHandle.Get(); }
	const D3D11_UAV_HANDLE GetUAV( void ) const { return m_UAVHandle[0].Get(); }

    bool IsTransparent() const;
	
	void SetClearColor( Color ClearColor ) { m_ClearColor = ClearColor; }

	void SetMsaaMode( uint32_t NumColorSamples, uint32_t NumCoverageSamples )
	{
		ASSERT(NumCoverageSamples >= NumColorSamples);
		m_FragmentCount = NumColorSamples;
		m_SampleCount = NumCoverageSamples;
	}

	Color GetClearColor( void ) const { return m_ClearColor; }

	// This will work for all texture sizes, but it's recommended for speed and quality
	// that you use dimensions with powers of two (but not necessarily square.)  Pass
	// 0 for ArrayCount to reserve space for mips at creation time.
	// void GenerateMipMaps( CommandContext& Context );

	void Destroy();

protected:

	// Compute the number of texture levels needed to reduce to 1x1.  This uses
	// _BitScanReverse to find the highest set bit.  Each dimension reduces by
	// half and truncates bits.  The dimension 256 (0x100) has 9 mip levels, same
	// as the dimension 511 (0x1FF).
	static inline uint32_t ComputeNumMips( uint32_t Width, uint32_t Height )
	{
		uint32_t HighBit;
		_BitScanReverse((unsigned long*)&HighBit, Width | Height);
		return HighBit + 1;
	}

	void CreateDerivedViews( ID3D11Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips = 1 );
    TextureDesc DescribeTex2D( uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT Format, uint32_t BindFlags );

	Color m_ClearColor;
	static const uint32_t kUAVSize = 12;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_SRVHandle;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_RTVHandle;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_UAVHandle[kUAVSize];
	uint32_t m_NumMipMaps; // number of texture sublevels
	uint32_t m_FragmentCount;
	uint32_t m_SampleCount;
};