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
#include "SamplerManager.h"
#include "GraphicsCore.h"
#include "Hash.h"
#include <map>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace Graphics;

namespace
{
	map< size_t, ComPtr<ID3D11SamplerState>> s_SamplerCache;
}

void SamplerDesc::DestroyAll()
{
	s_SamplerCache.clear();
}

D3D11_SAMPLER_HANDLE SamplerDesc::CreateDescriptor()
{
	size_t HashCode = Utility::HashState(this);

	ID3D11SamplerState** SamplerRef = nullptr;
	bool firstCompile = false;
	{
		static mutex s_HashMapMutex;
		lock_guard<mutex> CS( s_HashMapMutex );
		auto iter = s_SamplerCache.find( HashCode );

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == s_SamplerCache.end())
		{
			firstCompile = true;
			SamplerRef = s_SamplerCache[HashCode].GetAddressOf();
		}
		else
			SamplerRef = iter->second.GetAddressOf();
	}

	if (firstCompile)
	{
		ASSERT_SUCCEEDED( Graphics::g_Device->CreateSamplerState( this, &m_SamplerState) );
		s_SamplerCache[HashCode].Attach( m_SamplerState );
	}
	else
	{
		while (*SamplerRef == nullptr)
			this_thread::yield();
		m_SamplerState = *SamplerRef;
	}

	return m_SamplerState;
}
