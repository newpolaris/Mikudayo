#include "pch.h"
#include "Shader.h"
#include "InputLayout.h"
#include "GraphicsCore.h"
#include <boost/functional/hash.hpp>

using Microsoft::WRL::ComPtr;
using namespace std;

static map<size_t, ComPtr<ID3D11InputLayout>> s_InputLayoutHashMap;

std::size_t hash_value( const InputDesc& Desc )
{
	using namespace boost;

	// Offset is fixed by Format
	size_t HashCode = hash_value( Desc.SemanticName );
	hash_combine( HashCode, Desc.SemanticIndex );
	hash_combine( HashCode, Desc.Format );
	hash_combine( HashCode, Desc.InputSlotClass );
	hash_combine( HashCode, Desc.InstanceDataStepRate );

	return HashCode;
}

std::shared_ptr<InputLayout> InputLayout::Create( const std::vector<InputDesc>& Desc, const ShaderByteCode& Shader )
{
    ASSERT( Shader.pShaderBytecode != nullptr );
	std::shared_ptr<InputLayout> Ret;
	if (Desc.size() > 0)
	{
		auto IL = std::shared_ptr<InputLayout>(new InputLayout());
		IL->GetInputLayout( Desc, Shader );
		Ret.swap(IL);
	}
	return Ret;
}

void InputLayout::DestroyAll()
{
	s_InputLayoutHashMap.clear();
}

void InputLayout::GetInputLayout( const std::vector<InputDesc>& Desc, const ShaderByteCode& Shader )
{
	size_t HashCode = boost::hash_value(Desc);
	boost::hash_combine( HashCode, Shader.Name );

	ID3D11InputLayout** InputLayoutRef = nullptr;
	bool firstCompile = false;
	{
		static mutex s_HashMapMutex;
		lock_guard<mutex> CS( s_HashMapMutex );
		auto iter = s_InputLayoutHashMap.find( HashCode );

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == s_InputLayoutHashMap.end())
		{
			firstCompile = true;
			InputLayoutRef = s_InputLayoutHashMap[HashCode].GetAddressOf();
		}
		else
			InputLayoutRef = iter->second.GetAddressOf();
	}

	if (firstCompile)
	{
		D3D11_INPUT_ELEMENT_DESC ElemDesc[D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT] = {};
        for (size_t i = 0; i < Desc.size(); i++)
        {
            ElemDesc[i].SemanticName = Desc[i].SemanticName;
            ElemDesc[i].SemanticIndex = Desc[i].SemanticIndex;
            ElemDesc[i].Format = Desc[i].Format;
            ElemDesc[i].InputSlot = Desc[i].InputSlot;
            ElemDesc[i].AlignedByteOffset = Desc[i].AlignedByteOffset;
            ElemDesc[i].InputSlotClass = Desc[i].InputSlotClass;
            ElemDesc[i].InstanceDataStepRate = Desc[i].InstanceDataStepRate;
        }

		ASSERT_SUCCEEDED( Graphics::g_Device->CreateInputLayout( 
			ElemDesc,
			static_cast<UINT>( Desc.size() ),
			Shader.pShaderBytecode,
			Shader.Length,
			&m_InputLayout) );
		s_InputLayoutHashMap[HashCode].Attach( m_InputLayout );
	}
	else
	{
		while (*InputLayoutRef == nullptr)
			this_thread::yield();
		m_InputLayout = *InputLayoutRef;
	}
}

InputLayout::InputLayout() : m_InputLayout( nullptr )
{
}

void InputLayout::Bind( ID3D11DeviceContext* pContext )
{
	pContext->IASetInputLayout( m_InputLayout );
}
