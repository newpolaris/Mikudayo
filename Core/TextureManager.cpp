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
// Author(s):  James Stanard 
//             Alex Nankervis
//

#include "pch.h"
#include "TextureManager.h"
#include "GraphicsCore.h"
#include "FileUtility.h"
#include "CommandContext.h"
#include "WICTextureLoader.h"
#include "DDSTextureLoader.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <map>
#include <thread>
#include <sstream>

using namespace std;
using namespace Graphics;

//--------------------------------------------------------------------------------------
// Return the BPP for a particular format
//--------------------------------------------------------------------------------------
size_t BitsPerPixel( _In_ DXGI_FORMAT fmt )
{
    switch( fmt )
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
        return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return 24;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
        return 8;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 8;

    default:
        return 0;
    }
}

static UINT BytesPerPixel( DXGI_FORMAT Format )
{
	return (UINT)BitsPerPixel(Format) / 8;
};

void Texture::Create( size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitialData )
{
	uint32_t BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	UINT NumMips = 1;

	D3D11_TEXTURE2D_DESC Desc = {};
	Desc.Format = Format;
	Desc.Width = static_cast<UINT>(Width);
	Desc.Height = static_cast<UINT>(Height);
	Desc.ArraySize = 1;
	Desc.MipLevels = NumMips;
	Desc.Format = Format;
	Desc.BindFlags = BindFlags;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.CPUAccessFlags = 0;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;

	D3D11_SUBRESOURCE_DATA texResource;
	texResource.pSysMem = InitialData;
	texResource.SysMemPitch = static_cast<UINT>(Width * BytesPerPixel(Format));
	texResource.SysMemSlicePitch = static_cast<UINT>(texResource.SysMemPitch * Height);

	ComPtr<ID3D11Texture2D> Texture;
	ASSERT_SUCCEEDED( g_Device->CreateTexture2D( &Desc, &texResource, &Texture ) );
	Texture.As(&m_pResource);

	SetName(m_pResource, L"Texture");

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = Format;
	SRVDesc.Texture2D.MipLevels = NumMips;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

	g_Device->CreateShaderResourceView( m_pResource.Get(), &SRVDesc, m_SRV.ReleaseAndGetAddressOf() );
}

bool Texture::CreateWICFromMemory( const void* memBuffer, size_t bufferSize, bool sRGB )
{
	UINT loadFlag = sRGB ? DirectX::WIC_LOADER_FORCE_SRGB : DirectX::WIC_LOADER_DEFAULT;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;
	HRESULT hr = DirectX::CreateWICTextureFromMemoryEx( Graphics::g_Device,
		(uint8_t*)(memBuffer), bufferSize, 0, D3D11_USAGE_DEFAULT, 
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 
		0, D3D11_RESOURCE_MISC_GENERATE_MIPS, loadFlag, 
		m_pResource.GetAddressOf(), SRV.GetAddressOf());
	
	if (SUCCEEDED(hr))
	{
		GraphicsContext& gfxContext = GraphicsContext::Begin( L"MipMap" );
		gfxContext.GenerateMips( SRV.Get() );
		gfxContext.Finish();

		// WaitForLoad is depends on m_SRV member variable
		m_SRV.Swap(SRV);
	}
	return SUCCEEDED(hr);
}

bool Texture::CreateDDSFromMemory( const void* memBuffer, size_t bufferSize, bool sRGB )
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;
	HRESULT hr = DirectX::CreateDDSTextureFromMemoryEx( Graphics::g_Device,
		(uint8_t*)(memBuffer), bufferSize, 0, D3D11_USAGE_DEFAULT, 
		D3D11_BIND_SHADER_RESOURCE, 
		0, 0, sRGB, 
		m_pResource.GetAddressOf(), SRV.GetAddressOf());
	
	if (SUCCEEDED(hr))
	{
		// GraphicsContext& gfxContext = GraphicsContext::Begin( L"MipMap" );
		// gfxContext.GenerateMips( SRV.Get() );
		// gfxContext.Finish();

		// WaitForLoad is depends on m_SRV member variable
		m_SRV.Swap(SRV);
	}
	return SUCCEEDED(hr);
}

namespace TextureManager
{
	namespace fs = boost::filesystem;

	wstring s_RootPath = L"";
	map< wstring, unique_ptr<ManagedTexture> > s_TextureCache;

	void Initialize( const std::wstring& TextureLibRoot )
	{
		s_RootPath = TextureLibRoot;
	}

	void Shutdown( void )
	{
		s_TextureCache.clear();
	}

	std::wstring GetTexturePath( const std::wstring& filePath )
	{
		fs::path root(s_RootPath);
		root /= filePath;
		return root.generic_wstring();
	}

	pair<ManagedTexture*, bool> FindOrLoadTexture( const wstring& fileName )
	{
		static mutex s_Mutex;
		lock_guard<mutex> Guard(s_Mutex);

		auto iter = s_TextureCache.find( fileName );

		// If it's found, it has already been loaded or the load process has begun
		if (iter != s_TextureCache.end())
			return make_pair( iter->second.get(), false );

		ManagedTexture* NewTexture = new ManagedTexture( fileName );
		s_TextureCache[fileName].reset( NewTexture );

		// This was the first time it was requested, so indicate that the caller must read the file
		return make_pair(NewTexture, true);
	}

    const Texture& GetBlackTex2D(void)
    {
        auto ManagedTex = FindOrLoadTexture(L"DefaultBlackTexture");

        ManagedTexture* ManTex = ManagedTex.first;
        const bool RequestsLoad = ManagedTex.second;

        if (!RequestsLoad)
        {
            ManTex->WaitForLoad();
            return *ManTex;
        }

        uint32_t BlackPixel = 0;
        ManTex->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &BlackPixel);
        return *ManTex;
    }

    const Texture& GetWhiteTex2D(void)
    {
        auto ManagedTex = FindOrLoadTexture(L"DefaultWhiteTexture");

        ManagedTexture* ManTex = ManagedTex.first;
        const bool RequestsLoad = ManagedTex.second;

        if (!RequestsLoad)
        {
            ManTex->WaitForLoad();
            return *ManTex;
        }

        uint32_t WhitePixel = 0xFFFFFFFFul;
        ManTex->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &WhitePixel);
        return *ManTex;
    }

    const Texture& GetMagentaTex2D(void)
    {
        auto ManagedTex = FindOrLoadTexture(L"DefaultMagentaTexture");

        ManagedTexture* ManTex = ManagedTex.first;
        const bool RequestsLoad = ManagedTex.second;

        if (!RequestsLoad)
        {
            ManTex->WaitForLoad();
            return *ManTex;
        }

        uint32_t MagentaPixel = 0x00FF00FF;
        ManTex->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &MagentaPixel);
        return *ManTex;
    }

} // namespace TextureManager


void ManagedTexture::WaitForLoad( void ) const
{
	while (m_SRV == nullptr && m_IsValid)
		this_thread::yield();
}

const ManagedTexture* TextureManager::LoadFromFile( const wstring& fileName, bool sRGB )
{
	fs::path path( fileName );
	if (path.has_extension())
	{
		auto ext = path.extension().generic_wstring();

		const ManagedTexture* Tex = nullptr;
		if (boost::to_lower_copy(ext) == L".dds" )
			Tex = LoadDDSFromFile( fileName, sRGB );
		else
			Tex = LoadWISFromFile( fileName, sRGB );
		return Tex;
	}
	else
	{
		path.replace_extension( L".dds" );
		const ManagedTexture* Tex = nullptr;
		Tex = LoadDDSFromFile( fileName, sRGB );
		if (!Tex->IsValid())
		{
			path.replace_extension( L".png" );
			Tex = LoadWISFromFile( fileName, sRGB );
		}
		return Tex;
	}
}

void ManagedTexture::SetToInvalidTexture( void )
{
	// m_hCpuDescriptorHandle = TextureManager::GetMagentaTex2D().GetSRV();
	m_IsValid = false;
}

const ManagedTexture* TextureManager::LoadDDSFromFile( const wstring& fileName, bool sRGB )
{
	auto ManagedTex = FindOrLoadTexture( fileName );

	ManagedTexture* ManTex = ManagedTex.first;
	const bool RequestsLoad = ManagedTex.second;

	if (!RequestsLoad)
	{
		ManTex->WaitForLoad();
		return ManTex;
	}

	Utility::ByteArray ba = Utility::ReadFileSync( fileName );
	if (ba->size() == 0)
	{
		fs::path path( s_RootPath );
		path /= fileName;

		ba = Utility::ReadFileSync( path.generic_wstring() );
	}

	if (ba->size() == 0 || !ManTex->CreateDDSFromMemory( ba->data(), ba->size(), sRGB ))
		ManTex->SetToInvalidTexture();
	else
		SetName( ManTex->GetResource(), fileName );

	return ManTex;
}

const ManagedTexture* TextureManager::LoadWISFromFile( const wstring& fileName, bool sRGB )
{
	auto ManagedTex = FindOrLoadTexture( fileName );

	ManagedTexture* ManTex = ManagedTex.first;
	const bool RequestsLoad = ManagedTex.second;

	if (!RequestsLoad)
	{
		ManTex->WaitForLoad();
		return ManTex;
	}

	Utility::ByteArray ba = Utility::ReadFileSync( fileName );
	if (ba->size() == 0)
	{
		fs::path path( s_RootPath );
		path /= fileName;

		ba = Utility::ReadFileSync( path.generic_wstring() );
	}

	if (ba->size() == 0 || !ManTex->CreateWICFromMemory( ba->data(), ba->size(), sRGB ))
		ManTex->SetToInvalidTexture();

	SetName( ManTex->GetResource(), fileName );

	return ManTex;
}

const ManagedTexture* TextureManager::LoadFromStream( const std::wstring& key, std::istream& stream, bool sRGB )
{
	auto ManagedTex = FindOrLoadTexture( key );

	ManagedTexture* ManTex = ManagedTex.first;
	const bool RequestsLoad = ManagedTex.second;

	if (!RequestsLoad)
	{
		ManTex->WaitForLoad();
		return ManTex;
	}

   std::ostringstream ss;
   ss << stream.rdbuf();
   const std::string& s = ss.str();
   std::vector<char> buf(s.begin(), s.end());

   if (buf.size() == 0 ||
	   !(ManTex->CreateDDSFromMemory( buf.data(), buf.size(), sRGB ) ||
		   ManTex->CreateWICFromMemory( buf.data(), buf.size(), sRGB )))
	   ManTex->SetToInvalidTexture();
   else
	   SetName( ManTex->GetResource(), key );

   return ManTex;
}

