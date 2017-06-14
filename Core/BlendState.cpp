#include "pch.h"
#include "BlendState.h"
#include "GraphicsCore.h"
#include "Hash.h"

using Microsoft::WRL::ComPtr;
using namespace std;

static map<size_t, ComPtr<ID3D11BlendState>> s_RasterizerHashMap;

std::shared_ptr<BlendState> BlendState::Create( const BlendDesc& desc )
{
	auto Blend = std::shared_ptr<BlendState>(new BlendState());

	Blend->m_SampleMask = desc.SampleMask;
	std::copy_n(desc.BlendFactor, _countof(desc.BlendFactor), Blend->m_BlendFactor);

	Blend->GetBlendState( desc.Desc );
	
	return Blend;
}

void BlendState::DestroyAll()
{
	s_RasterizerHashMap.clear();
}

void BlendState::GetBlendState( const CD3D11_BLEND_DESC& desc )
{
	size_t HashCode = Utility::HashState( &desc );

	ID3D11BlendState** BlendRef = nullptr;
	bool firstCompile = false;
	{
		static mutex s_HashMapMutex;
		lock_guard<mutex> CS( s_HashMapMutex );
		auto iter = s_RasterizerHashMap.find( HashCode );

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == s_RasterizerHashMap.end())
		{
			firstCompile = true;
			BlendRef = s_RasterizerHashMap[HashCode].GetAddressOf();
		}
		else
			BlendRef = iter->second.GetAddressOf();
	}

	if (firstCompile)
	{
		ASSERT_SUCCEEDED( Graphics::g_Device->CreateBlendState( &desc, &m_BlendState ) );
		s_RasterizerHashMap[HashCode].Attach( m_BlendState );
	}
	else
	{
		while (*BlendRef == nullptr)
			this_thread::yield();
		m_BlendState = *BlendRef;
	}
}

BlendState::BlendState() : m_BlendState(nullptr) 
{
}

void BlendState::Bind( ID3D11DeviceContext* pContext )
{
	pContext->OMSetBlendState(m_BlendState, m_BlendFactor, m_SampleMask);
}
