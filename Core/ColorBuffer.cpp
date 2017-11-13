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
#include "GraphicsCore.h"
#include "ColorBuffer.h"
#include "DirectXTex.h" // HasAlpha

void ColorBuffer::CreateDerivedViews( ID3D11Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips )
{
	ASSERT(ArraySize == 1 || NumMips == 1, "We don't support auto-mips on texture arrays");

	m_NumMipMaps = NumMips - 1;

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

	RTVDesc.Format = Format;
	UAVDesc.Format = GetUAVFormat(Format);
	SRVDesc.Format = Format;

	if (ArraySize > 1)
	{
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		RTVDesc.Texture2DArray.MipSlice = 0;
		RTVDesc.Texture2DArray.FirstArraySlice = 0;
		RTVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

		UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		UAVDesc.Texture2DArray.MipSlice = 0;
		UAVDesc.Texture2DArray.FirstArraySlice = 0;
		UAVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		SRVDesc.Texture2DArray.MipLevels = NumMips;
		SRVDesc.Texture2DArray.MostDetailedMip = 0;
		SRVDesc.Texture2DArray.FirstArraySlice = 0;
		SRVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;
	}
	else if (m_FragmentCount > 1)
	{
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
	}
	else 
	{
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.MipSlice = 0;

		UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice = 0;

		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = NumMips;
		SRVDesc.Texture2D.MostDetailedMip = 0;
	}

	ID3D11Resource* Resource = m_pResource.Get();

	// Create the render target view
	Device->CreateRenderTargetView(Resource, &RTVDesc, m_RTVHandle.ReleaseAndGetAddressOf());

	// Create the shader resource view
	Device->CreateShaderResourceView(Resource, &SRVDesc, m_SRVHandle.ReleaseAndGetAddressOf());

    if (m_FragmentCount == 1)
    {
        // Create the UAVs for each mip level (RWTexture2D)
        for (uint32_t i = 0; i < kUAVSize; ++i)
            m_UAVHandle[i].Reset();
        for (uint32_t i = 0; i < NumMips; ++i)
        {
            Device->CreateUnorderedAccessView( Resource, &UAVDesc, m_UAVHandle[i].ReleaseAndGetAddressOf() );
            UAVDesc.Texture2D.MipSlice++;
        }
    }
}

void ColorBuffer::CreateFromSwapChain( const std::wstring& Name, Microsoft::WRL::ComPtr<ID3D11Texture2D> BaseResource )
{
	Microsoft::WRL::ComPtr<ID3D11Resource> Resource;
	BaseResource.As(&Resource);
	AssociateWithResource(Graphics::g_Device, Name, Resource );

	Graphics::g_Device->CreateRenderTargetView(m_pResource.Get(), nullptr, m_RTVHandle.ReleaseAndGetAddressOf());
}

void ColorBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format )
{
	NumMips = (NumMips == 0 ? ComputeNumMips( Width, Height ) : NumMips);
	
	uint32_t Flags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	if (m_FragmentCount == 1 && Format != DXGI_FORMAT_B5G6R5_UNORM)
		Flags |= D3D11_BIND_UNORDERED_ACCESS;

	TextureDesc Desc = DescribeTex2D( Width, Height, 1, NumMips, Format, Flags );
	CreateTextureResource( Graphics::g_Device, Name, Desc );
	CreateDerivedViews( Graphics::g_Device, Format, 1, NumMips );
}

void ColorBuffer::Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
	DXGI_FORMAT Format, EsramAllocator&)
{
	Create(Name, Width, Height, NumMips, Format);
}

void ColorBuffer::CreateArray( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
	DXGI_FORMAT Format )
{
	uint32_t Flags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	if (m_FragmentCount == 1)
		Flags |= D3D11_BIND_UNORDERED_ACCESS;

	TextureDesc Desc = DescribeTex2D( Width, Height, ArrayCount, 1, Format, Flags );

	CreateTextureResource(Graphics::g_Device, Name, Desc );
	CreateDerivedViews(Graphics::g_Device, Format, ArrayCount, 1);
}

void ColorBuffer::CreateArray( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
	DXGI_FORMAT Format, EsramAllocator& )
{
	CreateArray(Name, Width, Height, ArrayCount, Format);
}

TextureDesc ColorBuffer::DescribeTex2D( uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT Format, uint32_t BindFlags )
{
    TextureDesc Desc = PixelBuffer::DescribeTex2D( Width, Height, DepthOrArraySize, NumMips, Format, BindFlags );
    Desc.SampleDesc.Count =  m_FragmentCount;;
    Desc.SampleDesc.Quality = 0;
    return Desc;
}

void ColorBuffer::Destroy()
{
	m_SRVHandle.Reset();
	m_RTVHandle.Reset();
	for (uint32_t i = 0; i < kUAVSize; ++i)
		m_UAVHandle[i].Reset();
	PixelBuffer::Destroy();
}

bool ColorBuffer::IsTransparent() const
{
    return DirectX::HasAlpha( m_Format );
}
