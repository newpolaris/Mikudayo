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
#include "Mapping.h"
#include "Utility.h"

class Texture : public GpuResource
{
	friend class CommandContext;

public:

	Texture() { }

	// Create a 1-level 2D texture
	void Create( size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData );

	bool CreateWICFromMemory( const void* memBuffer, size_t bufferSize, bool sRGB = false );
	bool CreateDDSFromMemory( const void* memBuffer, size_t bufferSize, bool sRGB = false );

	void Destroy()
	{
		GpuResource::Destroy();
		m_SRV = nullptr;
	}

	const D3D11_SRV_HANDLE GetSRV() const { return m_SRV.Get(); }

	bool operator!() { return m_SRV == nullptr; }

protected:

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_SRV;
};

class ManagedTexture : public Texture
{
public:
	ManagedTexture( const std::wstring& FileName ) : m_MapKey(FileName), m_IsValid(true) {}

	void operator= ( const Texture& Texture );
	void Destroy() {
		// Wait until GenerateMips is completed
		WaitForLoad();
		Texture::Destroy();
	}

	void WaitForLoad( void ) const;
	void Unload(void);

	void SetToInvalidTexture(void);
	bool IsValid(void) const { return m_IsValid; }

private:
	std::wstring m_MapKey;		// For deleting from the map later
	bool m_IsValid;
};

namespace TextureManager
{
	void Initialize( const std::wstring& TextureLibRoot );
	void Shutdown(void);

	const ManagedTexture* LoadFromFile( const std::wstring& fileName, bool sRGB = false );
	const ManagedTexture* LoadDDSFromFile( const std::wstring& fileName, bool sRGB = false );
	const ManagedTexture* LoadWISFromFile( const std::wstring& fileName, bool sRGB = false );

	inline const ManagedTexture* LoadFromFile( const std::string& fileName, bool sRGB = false )
	{
		return LoadFromFile(MakeWStr(fileName), sRGB);
	}

	inline const ManagedTexture* LoadDDSFromFile( const std::string& fileName, bool sRGB = false )
	{
		return LoadDDSFromFile(MakeWStr(fileName), sRGB);
	}

	inline const ManagedTexture* LoadWISFromFile( const std::string& fileName, bool sRGB = false )
	{
		return LoadWISFromFile(MakeWStr(fileName), sRGB);
	}

	const Texture& GetBlackTex2D(void);
	const Texture& GetWhiteTex2D(void);
}
