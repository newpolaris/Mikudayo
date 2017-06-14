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

#include "GpuResource.h"

class EsramAllocator;

struct TextureDesc 
{
	uint32_t Width;
	uint32_t Height;
	uint16_t DepthOrArraySize;
	uint16_t MipLevels;
	uint32_t BindFlags;
	DXGI_FORMAT Format;
	D3D11_RESOURCE_DIMENSION Dimension;
	DXGI_SAMPLE_DESC SampleDesc;
};

class PixelBuffer : public GpuResource
{
public:
	PixelBuffer() : m_Width(0), m_Height(0), m_ArraySize(0), m_Format(DXGI_FORMAT_UNKNOWN), m_BankRotation(0) {}

	uint32_t GetWidth(void) const { return m_Width; }
	uint32_t GetHeight(void) const { return m_Height; }
	uint32_t GetDepth(void) const { return m_ArraySize; }
	const DXGI_FORMAT& GetFormat(void) const { return m_Format; }

	// Has no effect on Windows
	void SetBankRotation( uint32_t RotationAmount ) { m_BankRotation = RotationAmount; }

protected:

	TextureDesc DescribeTex2D( uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT Format, uint32_t BindFlags );

	void AssociateWithResource( ID3D11Device* Device, const std::wstring& Name, Microsoft::WRL::ComPtr<ID3D11Resource> Resource );

	void CreateTextureResource( ID3D11Device* Device, const std::wstring& Name, const TextureDesc& ResourceDesc );

	static DXGI_FORMAT GetBaseFormat( DXGI_FORMAT Format );
	static DXGI_FORMAT GetUAVFormat( DXGI_FORMAT Format );
	static DXGI_FORMAT GetDSVFormat( DXGI_FORMAT Format );
	static DXGI_FORMAT GetDepthFormat( DXGI_FORMAT Format );
	static DXGI_FORMAT GetStencilFormat( DXGI_FORMAT Format );

	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_ArraySize;
	DXGI_FORMAT m_Format;
	uint32_t m_BankRotation;
};

