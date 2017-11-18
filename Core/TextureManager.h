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

#pragma once

#include "pch.h"
#include "GpuResource.h"
#include "IColorBuffer.h"
#include "Mapping.h"
#include "Utility.h"
#include "TextUtility.h"
#include "FileUtility.h"

class Texture : public GpuResource, public IColorBuffer
{
	friend class CommandContext;

public:

    Texture();

	// Create a 1-level 2D texture
	void Create( size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData );

	bool CreateDDSFromMemory( const void* memBuffer, size_t bufferSize, bool sRGB = false );
	bool CreateHDRFromMemory( const void* memBuffer, size_t bufferSize, bool sRGB = false );
	bool CreateTGAFromMemory( const void* memBuffer, size_t bufferSize, bool sRGB = false );
	bool CreateWICFromMemory( const void* memBuffer, size_t bufferSize, bool sRGB = false );

	virtual void Destroy() override
	{
		GpuResource::Destroy();
		m_SRV = nullptr;
	}

	virtual const D3D11_SRV_HANDLE GetSRV() const override { return m_SRV.Get(); }
    void SetProperty( void );

	bool operator!() { return m_SRV == nullptr; }
    bool IsTransparent() const;

protected:

    bool m_bTransparent;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_SRV;
};

inline bool Texture::IsTransparent() const
{
    return m_bTransparent;
}

class ManagedTexture : public Texture
{
public:
    using Task = std::packaged_task<void(void)>;
	ManagedTexture( const std::wstring& FileName ) : m_MapKey(FileName), m_IsValid(true) {}
    virtual ~ManagedTexture();
	void operator= ( const Texture& Texture );
	virtual void Destroy() override {
		WaitForLoad();
		Texture::Destroy();
	}
	virtual const D3D11_SRV_HANDLE GetSRV() const override {
        return m_SRV.Get();
    }
    void SetTask( Task&& task );
	void WaitForLoad( void ) const;
    void Unload( void );
    bool IsValid( void ) const {
		WaitForLoad();
        return m_IsValid;
    }
    void SetToInvalidTexture( void );

private:

    std::thread m_LoadThread;
    std::future<void> m_Future;
	std::wstring m_MapKey;		// For deleting from the map later
	bool m_IsValid;
};

namespace TextureManager
{
	void Initialize( const std::wstring& TextureLibRoot );
	void Shutdown(void);

	const ManagedTexture* LoadFromFile( const std::wstring& fileName, bool sRGB = false );
	const ManagedTexture* LoadDDSFromFile( const std::wstring& fileName, bool sRGB = false );
	const ManagedTexture* LoadHDRFromFile( const std::wstring& fileName, bool sRGB = false );
	const ManagedTexture* LoadTGAFromFile( const std::wstring& fileName, bool sRGB = false );
	const ManagedTexture* LoadWISFromFile( const std::wstring& fileName, bool sRGB = false );
	const ManagedTexture* LoadFromStream( const std::wstring& key, std::istream& stream, bool sRGB = false );
    const ManagedTexture* LoadFromMemory( const std::wstring & key, size_t size, void * data, bool sRGB );
    const ManagedTexture* LoadFromMemory( const std::wstring& key, Utility::ByteArray ba, bool sRGB );

	inline const ManagedTexture* LoadFromFile( const std::string& fileName, bool sRGB = false )
	{
		return LoadFromFile(Utility::MakeWStr(fileName), sRGB);
	}

	inline const ManagedTexture* LoadDDSFromFile( const std::string& fileName, bool sRGB = false )
	{
		return LoadDDSFromFile(Utility::MakeWStr(fileName), sRGB);
	}

	inline const ManagedTexture* LoadHDRFromFile( const std::string& fileName, bool sRGB = false )
	{
		return LoadHDRFromFile(Utility::MakeWStr(fileName), sRGB);
	}

	inline const ManagedTexture* LoadWISFromFile( const std::string& fileName, bool sRGB = false )
	{
		return LoadWISFromFile(Utility::MakeWStr(fileName), sRGB);
	}

	const ManagedTexture& GetBlackTex2D(void);
	const ManagedTexture& GetWhiteTex2D(void);
    const ManagedTexture& GetMagentaTex2D(void);
}
