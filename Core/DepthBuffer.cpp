#include "pch.h"
#include "DepthBuffer.h"
#include "GraphicsCore.h"

using namespace Graphics;

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format )
{
	uint32_t Flags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	TextureDesc ResourceDesc = DescribeTex2D( Width, Height, 1, 1, Format, Flags );

	CreateTextureResource(Graphics::g_Device, Name, ResourceDesc);
	CreateDerivedViews(Graphics::g_Device, Format);
}

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t Samples, DXGI_FORMAT Format )
{
	uint32_t Flags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	TextureDesc ResourceDesc = DescribeTex2D(Width, Height, 1, 1, Format, Flags );
	ResourceDesc.SampleDesc.Count = m_FragmentCount = Samples;

	CreateTextureResource(Graphics::g_Device, Name, ResourceDesc);
	CreateDerivedViews(Graphics::g_Device, Format);
}

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, EsramAllocator& )
{
	Create(Name, Width, Height, Format);
}

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t Samples, DXGI_FORMAT Format, EsramAllocator& )
{
	Create(Name, Width, Height, Samples, Format);
}

void DepthBuffer::CreateDerivedViews( ID3D11Device* Device, DXGI_FORMAT Format )
{
	ID3D11Resource* Resource = m_pResource.Get();

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = GetDSVFormat(Format);
	if (m_FragmentCount == 1)
	{
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	}
	else
	{
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	}

	dsvDesc.Flags = 0;
	Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSVHandle[0].ReleaseAndGetAddressOf());

	dsvDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
	Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSVHandle[1].ReleaseAndGetAddressOf());

	DXGI_FORMAT stencilReadFormat = GetStencilFormat(Format);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		dsvDesc.Flags = D3D11_DSV_READ_ONLY_STENCIL;
		Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSVHandle[2].ReleaseAndGetAddressOf());

		dsvDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;
		Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSVHandle[3].ReleaseAndGetAddressOf());
	}
	else
	{
		m_hDSVHandle[2] = m_hDSVHandle[0];
		m_hDSVHandle[3] = m_hDSVHandle[1];
	}

	// TO HANDLE VS2015 BUG. Save pointer another member variable
	for (uint32_t i = 0; i < kDSVSize; ++i) 
		m_hDSV[i] = m_hDSVHandle[i].Get();

	// Create the shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = GetDepthFormat(Format);
	if (dsvDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2D)
	{
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;
	}
	else
	{
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
	}
	Device->CreateShaderResourceView( Resource, &SRVDesc, m_hDepthSRV.ReleaseAndGetAddressOf() );

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		SRVDesc.Format = stencilReadFormat;
		Device->CreateShaderResourceView( Resource, &SRVDesc, m_hStencilSRV.ReleaseAndGetAddressOf() );
	}
}

void DepthBuffer::Destroy()
{
	m_hDepthSRV.Reset();
	m_hStencilSRV.Reset();
	for (uint32_t i = 0; i < kDSVSize; ++i) 
	{
		m_hDSVHandle[i].Reset();
		m_hDSV[i] = nullptr;
	}
		

	PixelBuffer::Destroy();
}
