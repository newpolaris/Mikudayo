#include "pch.h"
#include "DepthStencilState.h"
#include "GraphicsCore.h"
#include "Hash.h"

using Microsoft::WRL::ComPtr;
using namespace std;

static map<size_t, ComPtr<ID3D11DepthStencilState>> s_DepthStencilHashMap;

std::shared_ptr<DepthStencilState> DepthStencilState::Create( const DepthStencilDesc& desc )
{
	auto DepthStencil = std::shared_ptr<DepthStencilState>(new DepthStencilState());

	DepthStencil->m_StencilRef = desc.StencilRef;
	DepthStencil->GetState( desc.Desc );
	
	return DepthStencil;
}

void DepthStencilState::DestroyAll()
{
	s_DepthStencilHashMap.clear();
}

void DepthStencilState::GetState( const CD3D11_DEPTH_STENCIL_DESC& desc )
{
	size_t HashCode = Utility::HashState( &desc );

	ID3D11DepthStencilState** DpethStencilRef = nullptr;
	bool firstCompile = false;
	{
		static mutex s_HashMapMutex;
		lock_guard<mutex> CS( s_HashMapMutex );
		auto iter = s_DepthStencilHashMap.find( HashCode );

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == s_DepthStencilHashMap.end())
		{
			firstCompile = true;
			DpethStencilRef = s_DepthStencilHashMap[HashCode].GetAddressOf();
		}
		else
			DpethStencilRef = iter->second.GetAddressOf();
	}

	if (firstCompile)
	{
		ASSERT_SUCCEEDED( Graphics::g_Device->CreateDepthStencilState( &desc, &m_DepthStencilState ) );
		s_DepthStencilHashMap[HashCode].Attach( m_DepthStencilState );
	}
	else
	{
		while (*DpethStencilRef == nullptr)
			this_thread::yield();
		m_DepthStencilState = *DpethStencilRef;
	}
}

DepthStencilState::DepthStencilState() : m_DepthStencilState(nullptr) 
{
}

void DepthStencilState::Bind( ID3D11DeviceContext* pContext )
{
	pContext->OMSetDepthStencilState(m_DepthStencilState, m_StencilRef);
}
