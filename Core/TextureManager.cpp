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
#include "DirectXTex.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <map>
#include <thread>
#include <sstream>

using namespace std;
using namespace Graphics;

static UINT BytesPerPixel( DXGI_FORMAT Format )
{
	return (UINT)DirectX::BitsPerPixel(Format) / 8;
}

Texture::Texture() : m_bTransparent(false) 
{
}

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
    SetProperty();

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
    GraphicsContext& gfxContext = GraphicsContext::Begin();
    // Load texture and generate mips at the same time
	HRESULT hr = DirectX::CreateWICTextureFromMemoryEx(
        Graphics::g_Device,
        gfxContext.m_CommandList,
		(uint8_t*)(memBuffer), bufferSize, 0, D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
		0, D3D11_RESOURCE_MISC_GENERATE_MIPS, loadFlag,
		m_pResource.GetAddressOf(), m_SRV.GetAddressOf());
    gfxContext.Finish();

	return SUCCEEDED( hr );
}

bool Texture::CreateTGAFromMemory( const void* memBuffer, size_t bufferSize, bool sRGB )
{
    DirectX::ScratchImage image, mipChain;
    HRESULT hr = DirectX::LoadFromTGAMemory( memBuffer, bufferSize, nullptr, image );
    if (SUCCEEDED(hr))
        hr = DirectX::GenerateMipMaps( image.GetImages(), image.GetImageCount(), image.GetMetadata(), TEX_FILTER_DEFAULT, 0, mipChain );
    if (SUCCEEDED(hr))
        hr = DirectX::CreateShaderResourceViewEx( Graphics::g_Device, mipChain.GetImages(),
            mipChain.GetImageCount(), mipChain.GetMetadata(),
            D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
            0, 0, sRGB, m_pResource.GetAddressOf(), m_SRV.GetAddressOf() );
	return SUCCEEDED( hr );
}

bool Texture::CreateDDSFromMemory( const void* memBuffer, size_t bufferSize, bool sRGB )
{
	HRESULT hr = DirectX::CreateDDSTextureFromMemoryEx( Graphics::g_Device,
		(uint8_t*)(memBuffer), bufferSize, 0, D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE,
		0, 0, sRGB,
		m_pResource.GetAddressOf(), m_SRV.GetAddressOf());

	if (SUCCEEDED(hr))
	{
        // Can't generate mips. Fail to create resource
        //
		// GraphicsContext& gfxContext = GraphicsContext::Begin( L"MipMap" );
		// gfxContext.GenerateMips( SRV.Get() );
		// gfxContext.Finish();
	}
	return SUCCEEDED(hr);
}

void Texture::SetProperty( void )
{
    if (m_pResource) 
    {
        D3D11_RESOURCE_DIMENSION type;
        m_pResource->GetType( &type );
        if (type == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
        {
            ID3D11Texture1D* texture = (ID3D11Texture1D*)m_pResource.Get();
            D3D11_TEXTURE1D_DESC desc;
            texture->GetDesc( &desc );
            m_bTransparent = HasAlpha( desc.Format );
        }
        else if (type == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        {
            ID3D11Texture2D* texture = (ID3D11Texture2D*)m_pResource.Get();
            D3D11_TEXTURE2D_DESC desc;
            texture->GetDesc( &desc );
            m_bTransparent = HasAlpha( desc.Format );
        }
        else if (type == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
        {
            ID3D11Texture3D* texture = (ID3D11Texture3D*)m_pResource.Get();
            D3D11_TEXTURE3D_DESC desc;
            texture->GetDesc( &desc );
            m_bTransparent = HasAlpha( desc.Format );
        }
    }
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

    const ManagedTexture& GetBlackTex2D(void)
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

    const ManagedTexture& GetWhiteTex2D(void)
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

    const ManagedTexture& GetMagentaTex2D(void)
    {
        auto ManagedTex = FindOrLoadTexture(L"DefaultMagentaTexture");

        ManagedTexture* ManTex = ManagedTex.first;
        const bool RequestsLoad = ManagedTex.second;

        if (!RequestsLoad)
        {
            ManTex->WaitForLoad();
            return *ManTex;
        }

        uint32_t MagentaPixel = 0xFFFF00FF;
        ManTex->Create(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &MagentaPixel);
        return *ManTex;
    }

} // namespace TextureManager

ManagedTexture::~ManagedTexture()
{
    if (m_LoadThread.joinable())
        m_LoadThread.join();
    if (m_Future.valid())
        m_Future.get();
}

void ManagedTexture::SetTask( Task&& task )
{
    m_Future = task.get_future();
    std::thread thread( std::forward<Task>(task) );
    m_LoadThread.swap(thread);
}

void ManagedTexture::WaitForLoad( void ) const
{
    auto pointer = const_cast<ManagedTexture*>(this);
    if (m_LoadThread.joinable())
        pointer->m_LoadThread.join();
    if (m_Future.valid())
        pointer->m_Future.get();
}

const ManagedTexture* TextureManager::LoadFromFile( const wstring& fileName, bool sRGB )
{
	fs::path path( fileName );
	if (path.has_extension())
	{
		auto ext = boost::to_lower_copy(path.extension().generic_wstring());
		const ManagedTexture* Tex = nullptr;
		if ( ext == L".dds" )
			Tex = LoadDDSFromFile( fileName, sRGB );
        else if ( ext == L".tga" )
			Tex = LoadTGAFromFile( fileName, sRGB );
        else
			Tex = LoadWISFromFile( fileName, sRGB );
		return Tex;
	}
	else
	{
		const ManagedTexture* Tex = nullptr;
		path.replace_extension( L".dds" );
        if (fs::exists(path) || fs::exists(s_RootPath/path))
            Tex = LoadDDSFromFile( path.generic_wstring(), sRGB );
		path.replace_extension( L".tga" );
        if (fs::exists(path) || fs::exists(s_RootPath/path))
            Tex = LoadTGAFromFile( path.generic_wstring(), sRGB );
		path.replace_extension( L".png" );
        Tex = LoadWISFromFile( path.generic_wstring(), sRGB );
		return Tex;
	}
}

void ManagedTexture::SetToInvalidTexture( void )
{
	m_SRV = TextureManager::GetMagentaTex2D().m_SRV;
	m_IsValid = false;
}

const ManagedTexture* TextureManager::LoadDDSFromFile( const wstring& fileName, bool sRGB )
{
    ASSERT( !s_RootPath.empty() );
	auto ManagedTex = FindOrLoadTexture( fileName );

	ManagedTexture* ManTex = ManagedTex.first;
	const bool RequestsLoad = ManagedTex.second;

	if (!RequestsLoad)
	{
		ManTex->WaitForLoad();
		return ManTex;
	}

    ManagedTexture::Task task( [=]
    {
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
        ManTex->SetProperty();
    });
    ManTex->SetTask( std::move( task ));
    return ManTex;
}

const ManagedTexture* TextureManager::LoadTGAFromFile( const wstring& fileName, bool sRGB )
{
	auto ManagedTex = FindOrLoadTexture( fileName );

	ManagedTexture* ManTex = ManagedTex.first;
	const bool RequestsLoad = ManagedTex.second;

	if (!RequestsLoad)
	{
		ManTex->WaitForLoad();
		return ManTex;
	}

    ManagedTexture::Task task( [=]
    {
        Utility::ByteArray ba = Utility::ReadFileSync( fileName );
        if (ba->size() == 0)
        {
            fs::path path( s_RootPath );
            path /= fileName;

            ba = Utility::ReadFileSync( path.generic_wstring() );
        }

        if (ba->size() == 0 || !ManTex->CreateTGAFromMemory( ba->data(), ba->size(), sRGB ))
            ManTex->SetToInvalidTexture();
        else
            SetName( ManTex->GetResource(), fileName );
        ManTex->SetProperty();
    });
    ManTex->SetTask( std::move(task) );

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

    ManagedTexture::Task task( [=]
    {
        Utility::ByteArray ba = Utility::ReadFileSync( fileName );
        if (ba->size() == 0)
        {
            fs::path path( s_RootPath );
            path /= fileName;

            ba = Utility::ReadFileSync( path.generic_wstring() );
        }

        if (ba->size() == 0 || !ManTex->CreateWICFromMemory( ba->data(), ba->size(), sRGB ))
            ManTex->SetToInvalidTexture();
        else
            SetName( ManTex->GetResource(), fileName );
        ManTex->SetProperty();
    });
    ManTex->SetTask( std::move(task) );

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
    std::vector<char> buf( s.begin(), s.end() );

    ManagedTexture::Task task( [cbuf = std::move(buf), sRGB, ManTex, key]
    {
        if (cbuf.size() == 0 ||
            !(ManTex->CreateDDSFromMemory( cbuf.data(), cbuf.size(), sRGB ) ||
                ManTex->CreateWICFromMemory( cbuf.data(), cbuf.size(), sRGB ) ||
                ManTex->CreateTGAFromMemory( cbuf.data(), cbuf.size(), sRGB )))
            ManTex->SetToInvalidTexture();
        else
            SetName( ManTex->GetResource(), key );
        ManTex->SetProperty();
    });
    ManTex->SetTask( std::move(task) );
    return ManTex;
}

const ManagedTexture* TextureManager::LoadFromMemory( const std::wstring& key, Utility::ByteArray ba, bool sRGB )
{
	auto ManagedTex = FindOrLoadTexture( key );

	ManagedTexture* ManTex = ManagedTex.first;
	const bool RequestsLoad = ManagedTex.second;

	if (!RequestsLoad)
	{
		ManTex->WaitForLoad();
		return ManTex;
	}

    ManagedTexture::Task task( [=]
    {
        if (ba->size() == 0 ||
            !(ManTex->CreateDDSFromMemory( ba->data(), ba->size(), sRGB ) ||
                ManTex->CreateWICFromMemory( ba->data(), ba->size(), sRGB ) ||
                ManTex->CreateTGAFromMemory( ba->data(), ba->size(), sRGB )))
            ManTex->SetToInvalidTexture();
        else
            SetName( ManTex->GetResource(), key );
        ManTex->SetProperty();
    } );
    ManTex->SetTask( std::move(task) );

    return ManTex;
}

const ManagedTexture* TextureManager::LoadFromMemory( const std::wstring& key, size_t size, void* data, bool sRGB )
{
	auto ManagedTex = FindOrLoadTexture( key );

	ManagedTexture* ManTex = ManagedTex.first;
	const bool RequestsLoad = ManagedTex.second;

	if (!RequestsLoad)
	{
		ManTex->WaitForLoad();
		return ManTex;
	}

    std::vector<char> buf( (char*)data, (char*)data + size );

    ManagedTexture::Task task( [cbuf = std::move(buf), sRGB, ManTex, key, size]
    {
        if (size == 0 ||
            !(ManTex->CreateDDSFromMemory( cbuf.data(), size, sRGB ) ||
                ManTex->CreateWICFromMemory( cbuf.data(), size, sRGB ) ||
                ManTex->CreateTGAFromMemory( cbuf.data(), size, sRGB )))
            ManTex->SetToInvalidTexture();
        else
            SetName( ManTex->GetResource(), key );
        ManTex->SetProperty();
    });
    ManTex->SetTask( std::move(task) );
    return ManTex;
}
