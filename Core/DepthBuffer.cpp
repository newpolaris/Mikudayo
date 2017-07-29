#include "pch.h"
#include "DepthBuffer.h"
#include "GraphicsCore.h"

using namespace Graphics;

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format )
{
	uint32_t Flags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	TextureDesc ResourceDesc = DescribeTex2D( Width, Height, 1, 1, Format, Flags );

	CreateTextureResource(Graphics::g_Device, Name, ResourceDesc);
	CreateDerivedViews(Graphics::g_Device, Format, 1);
}

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t Samples, DXGI_FORMAT Format )
{
	uint32_t Flags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	TextureDesc ResourceDesc = DescribeTex2D(Width, Height, 1, 1, Format, Flags );
	ResourceDesc.SampleDesc.Count = m_FragmentCount = Samples;

	CreateTextureResource(Graphics::g_Device, Name, ResourceDesc);
	CreateDerivedViews(Graphics::g_Device, Format, 1);
}

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, EsramAllocator& )
{
	Create(Name, Width, Height, Format);
}

void DepthBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t Samples, DXGI_FORMAT Format, EsramAllocator& )
{
	Create(Name, Width, Height, Samples, Format);
}

void DepthBuffer::CreateArray( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount, DXGI_FORMAT Format )
{
	uint32_t Flags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	TextureDesc ResourceDesc = DescribeTex2D( Width, Height, ArrayCount, 1, Format, Flags );

	CreateTextureResource(Graphics::g_Device, Name, ResourceDesc);
	CreateDerivedViews(Graphics::g_Device, Format, ArrayCount);
}

void DepthBuffer::CreateArray( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount, DXGI_FORMAT Format, EsramAllocator& )
{
    CreateArray( Name, Width, Height, ArrayCount, Format );
}

void DepthBuffer::CreateDerivedViews( ID3D11Device* Device, DXGI_FORMAT Format, uint32_t ArraySize )
{
	ID3D11Resource* Resource = m_pResource.Get();

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = GetDSVFormat(Format);
    if (ArraySize > 1)
    {
        if (m_FragmentCount == 1)
        {
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice = 0;
            dsvDesc.Texture2DArray.FirstArraySlice = 0;
            dsvDesc.Texture2DArray.ArraySize = ArraySize;
        }
        else
        {
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
            dsvDesc.Texture2DMSArray.FirstArraySlice = 0;
            dsvDesc.Texture2DMSArray.ArraySize = ArraySize;
        }
    }
    else
    {
        if (m_FragmentCount == 1)
        {
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = 0;
        }
        else
        {
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        }
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
    if (ArraySize > 1)
    {
        if (dsvDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DARRAY)
        {
            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            SRVDesc.Texture2DArray.MipLevels = 1;
            SRVDesc.Texture2DArray.FirstArraySlice = 0;
            SRVDesc.Texture2DArray.ArraySize = ArraySize;
        }
        else
        {
            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            SRVDesc.Texture2DMSArray.FirstArraySlice = 0;
            SRVDesc.Texture2DMSArray.ArraySize = ArraySize;
        }
    }
    else
    {
        if (dsvDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2D)
        {
            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            SRVDesc.Texture2D.MipLevels = 1;
        }
        else
        {
            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
        }
    }
	Device->CreateShaderResourceView( Resource, &SRVDesc, m_hDepthSRV.ReleaseAndGetAddressOf() );

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		SRVDesc.Format = stencilReadFormat;
		Device->CreateShaderResourceView( Resource, &SRVDesc, m_hStencilSRV.ReleaseAndGetAddressOf() );
	}

    if (ArraySize > 1)
    {
        for (uint32_t i = 0; i < ArraySize; i++)
        {
            if (m_FragmentCount == 1)
            {
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                dsvDesc.Texture2DArray.MipSlice = 0;
                dsvDesc.Texture2DArray.FirstArraySlice = i;
                dsvDesc.Texture2DArray.ArraySize = 1;
            }
            else
            {
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                dsvDesc.Texture2DMSArray.FirstArraySlice = i;
                dsvDesc.Texture2DMSArray.ArraySize = 1;
            }
            dsvDesc.Flags = 0;
            Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DSV;
            Device->CreateDepthStencilView( Resource, &dsvDesc, DSV.GetAddressOf() );
            m_hDSVSliceHandle.push_back( DSV );

            if (dsvDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DARRAY)
            {
                SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                SRVDesc.Texture2DArray.MipLevels = 1;
                SRVDesc.Texture2DArray.FirstArraySlice = i;
                SRVDesc.Texture2DArray.ArraySize = 1;
            }
            else
            {
                SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                SRVDesc.Texture2DMSArray.FirstArraySlice = i;
                SRVDesc.Texture2DMSArray.ArraySize = 1;
            }
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;
            Device->CreateShaderResourceView( Resource, &SRVDesc, SRV.GetAddressOf() );
            m_hSRVSliceHandle.push_back( SRV);
        }
        // TO HANDLE VS2015 BUG. Save pointer another member variable
        for (auto i = 0; i < m_hDSVSliceHandle.size(); ++i)
            m_hDSVSlice.push_back( m_hDSVSliceHandle[i].Get() );
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
    m_hDSVSliceHandle.clear();
    m_hDSVSlice.clear();
    m_hSRVSliceHandle.clear();

	PixelBuffer::Destroy();
}
