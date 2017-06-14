#include "pch.h"
#include "RasterizerState.h"
#include "GraphicsCore.h"
#include "Hash.h"

using Microsoft::WRL::ComPtr;
using namespace std;

static map<size_t, ComPtr<ID3D11RasterizerState>> s_RasterizerHashMap;

std::shared_ptr<RasterizerState> RasterizerState::Create( const RasterizerDesc& desc )
{
	auto Rasterizer = std::shared_ptr<RasterizerState>(new RasterizerState());
	Rasterizer->GetState( desc.Desc );
	
	return Rasterizer;
}

void RasterizerState::DestroyAll()
{
	s_RasterizerHashMap.clear();
}

void RasterizerState::GetState( const CD3D11_RASTERIZER_DESC& desc )
{
	size_t HashCode = Utility::HashState( &desc );

	ID3D11RasterizerState** RasterizerRef = nullptr;
	bool firstCompile = false;
	{
		static mutex s_HashMapMutex;
		lock_guard<mutex> CS( s_HashMapMutex );
		auto iter = s_RasterizerHashMap.find( HashCode );

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == s_RasterizerHashMap.end())
		{
			firstCompile = true;
			RasterizerRef = s_RasterizerHashMap[HashCode].GetAddressOf();
		}
		else
			RasterizerRef = iter->second.GetAddressOf();
	}

	if (firstCompile)
	{
		ASSERT_SUCCEEDED( Graphics::g_Device->CreateRasterizerState( &desc, &m_RasterizerState ) );
		s_RasterizerHashMap[HashCode].Attach( m_RasterizerState );
	}
	else
	{
		while (*RasterizerRef == nullptr)
			this_thread::yield();
		m_RasterizerState = *RasterizerRef;
	}
}

RasterizerState::RasterizerState() : m_RasterizerState(nullptr) 
{
}

void RasterizerState::Bind( ID3D11DeviceContext* pContext )
{
	pContext->RSSetState( m_RasterizerState );
}
