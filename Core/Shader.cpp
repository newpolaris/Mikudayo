//--------------------------------------------------------------------------------
// This file is a portion of the Hieroglyph 3 Rendering Engine.  It is distributed
// under the MIT License, available in the root of this distribution and
// at the following URL:
//
// http://www.opensource.org/licenses/mit-license.php
//
// Copyright (c) Jason Zink
//--------------------------------------------------------------------------------

#include "pch.h"
#include "Shader.h"
#include "GraphicsCore.h"
#include "TextUtility.h"
#include "GpuResource.h"

using namespace std;
using Microsoft::WRL::ComPtr;
using Graphics::g_Device;

static std::map<std::wstring, std::shared_ptr<Shader>> s_ShaderMap;

std::shared_ptr<Shader> Shader::Empty[] = {
    std::shared_ptr<Shader>( new Shader( kVertexShader, L"EmptyVS" )),
    std::shared_ptr<Shader>( new Shader( kHullShader, L"EmptyHS" )),
    std::shared_ptr<Shader>( new Shader( kDomainShader, L"EmptyDS" )),
    std::shared_ptr<Shader>( new Shader( kGeometryShader, L"EmptyGS" )),
    std::shared_ptr<Shader>( new Shader( kPixelShader, L"EmptyPS" )),
    std::shared_ptr<Shader>( new Shader( kComputeShader, L"EmptyCS" )),
};

std::shared_ptr<Shader> Shader::Create( ShaderType Type, const ShaderByteCode& ByteCode )
{
	if (!ByteCode.valid())
		return Empty[Type];

	static mutex s_HashMapMutex;
	lock_guard<mutex> CS( s_HashMapMutex );
	auto iter = s_ShaderMap.find( ByteCode.Name );

	// Reserve space so the next inquiry will find that someone got here first.
	if (iter == s_ShaderMap.end())
	{
		auto shader = std::shared_ptr<Shader>(new Shader(Type));
		shader->Create(ByteCode);
		s_ShaderMap.insert( { ByteCode.Name, shader } );
		return shader;
	}
	else
		return iter->second;
}

void Shader::DestroyAll()
{
	s_ShaderMap.clear();
}

Shader::Shader( ShaderType Type ) : m_ShaderType(Type)
{
}

Shader::Shader( ShaderType Type, std::wstring Name ) : m_ShaderType(Type), m_Name(Name)
{
}

Shader::~Shader()
{
}

void Shader::Create( const ShaderByteCode& ByteCode )
{
	ASSERT(!m_Shader);

	switch (m_ShaderType) {
		case kVertexShader:
		{
			ComPtr<ID3D11VertexShader> Shader;
            ASSERT_SUCCEEDED(g_Device->CreateVertexShader( ByteCode.pShaderBytecode, ByteCode.Length, nullptr, Shader.GetAddressOf()));
            m_Shader.Swap(Shader);
            break;
		}
		case kPixelShader:
		{
			ComPtr<ID3D11PixelShader> Shader;
			ASSERT_SUCCEEDED(g_Device->CreatePixelShader( ByteCode.pShaderBytecode, ByteCode.Length, nullptr, Shader.GetAddressOf()));
			m_Shader.Swap(Shader);
            break;
		}
		case kGeometryShader:
		{
			ComPtr<ID3D11GeometryShader> Shader;
			ASSERT_SUCCEEDED(g_Device->CreateGeometryShader( ByteCode.pShaderBytecode, ByteCode.Length, nullptr, Shader.GetAddressOf()));
			m_Shader.Swap(Shader);
            break;
		}
        case kDomainShader:
        {
			ComPtr<ID3D11GeometryShader> Shader;
			ASSERT_SUCCEEDED(g_Device->CreateGeometryShader( ByteCode.pShaderBytecode, ByteCode.Length, nullptr, Shader.GetAddressOf()));
			m_Shader.Swap(Shader);
            break;
        }
        case kHullShader:
        {
			ComPtr<ID3D11HullShader> Shader;
			ASSERT_SUCCEEDED(g_Device->CreateHullShader( ByteCode.pShaderBytecode, ByteCode.Length, nullptr, Shader.GetAddressOf()));
			m_Shader.Swap(Shader);
            break;
        }
		case kComputeShader:
		{
			ComPtr<ID3D11ComputeShader> Shader;
			ASSERT_SUCCEEDED(g_Device->CreateComputeShader( ByteCode.pShaderBytecode, ByteCode.Length, nullptr, Shader.GetAddressOf()));
			m_Shader.Swap(Shader);
            break;
		}
	};

	m_ShaderByteCode = ByteCode;
    m_Name = ByteCode.Name;
    SetName( m_Shader, ByteCode.Name );
	FillReflection();
}

void Shader::FillReflection()
{
	ComPtr<ID3D11ShaderReflection> pReflector;
	ASSERT_SUCCEEDED( D3DReflect( m_ShaderByteCode.pShaderBytecode, m_ShaderByteCode.Length,
		IID_ID3D11ShaderReflection, reinterpret_cast<void**>(pReflector.GetAddressOf()) ) );

	// Get the top level shader information, including the number of constant buffers,
	// as well as the number bound resources (constant buffers + other objects), the
	// number of input elements, and the number of output elements for the shader.

	D3D11_SHADER_DESC desc;
	pReflector->GetDesc( &desc );

	// Get the input and output signature description arrays.  These can be used later
	// for verification of linking shader stages together.
	for ( UINT i = 0; i < desc.InputParameters; i++ )
	{
		D3D11_SIGNATURE_PARAMETER_DESC input_desc;
		pReflector->GetInputParameterDesc( i, &input_desc );
		m_InputSignatureParameters.push_back( input_desc );
	}
	for ( UINT i = 0; i < desc.OutputParameters; i++ )
	{
		D3D11_SIGNATURE_PARAMETER_DESC output_desc;
		pReflector->GetOutputParameterDesc( i, &output_desc );
		m_OutputSignatureParameters.push_back( output_desc );
	}

	// Get the constant buffer information, which will be used for setting parameters
	// for use by this shader at rendering time.
	for ( UINT i = 0; i < desc.ConstantBuffers; i++ )
	{
		ID3D11ShaderReflectionConstantBuffer* pConstBuffer = pReflector->GetConstantBufferByIndex( i );
		ConstantBufferLayout BufferLayout;

		D3D11_SHADER_BUFFER_DESC bufferDesc;
		pConstBuffer->GetDesc( &bufferDesc );
		BufferLayout.Description = bufferDesc;

		// Load the description of each variable for use later on when binding a buffer
		for (UINT j = 0; j < BufferLayout.Description.Variables; j++)
		{
			// Get the variable description and store it
			ID3D11ShaderReflectionVariable* pVariable = pConstBuffer->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC var_desc;
			pVariable->GetDesc(&var_desc);

			BufferLayout.Variables.push_back(var_desc);

			// Get the variable type description and store it
			ID3D11ShaderReflectionType* pType = pVariable->GetType();
			D3D11_SHADER_TYPE_DESC type_desc;
			pType->GetDesc(&type_desc);

			BufferLayout.Types.push_back(type_desc);
		}
		m_BufferDescription.push_back(BufferLayout);
	}

	for ( UINT i = 0; i < desc.BoundResources; i++ )
	{
		D3D11_SHADER_INPUT_BIND_DESC resource_desc;
		pReflector->GetResourceBindingDesc( i, &resource_desc );
		m_ResourceDescrition.push_back({ resource_desc });
	}
}

bool Shader::ShaderCheckResource(D3D_SHADER_INPUT_TYPE inputType, UINT slot, std::string name)
{
	for (UINT i = 0; i < m_ResourceDescrition.size(); i++) {
		if (m_ResourceDescrition[i].Type == inputType && m_ResourceDescrition[i].BindPoint == slot) {
			ASSERT(m_ResourceDescrition[i].Name == name);
			if (m_ResourceDescrition[i].Name == name)
				return true;
		}
	}
	return false;
}

void Shader::Bind( ID3D11DeviceContext* pContext )
{
    auto shader = m_Shader != nullptr ? m_Shader.Get() : nullptr;
    switch (m_ShaderType) {
    case kVertexShader:
        pContext->VSSetShader( reinterpret_cast<ID3D11VertexShader*>(shader), nullptr, 0 );
        break;
    case kPixelShader:
        pContext->PSSetShader( reinterpret_cast<ID3D11PixelShader*>(shader), nullptr, 0 );
        break;
    case kGeometryShader:
        pContext->GSSetShader( reinterpret_cast<ID3D11GeometryShader*>(shader), nullptr, 0 );
        break;
    case kDomainShader:
        pContext->DSSetShader( reinterpret_cast<ID3D11DomainShader*>(shader), nullptr, 0 );
        break;
    case kHullShader:
        pContext->HSSetShader( reinterpret_cast<ID3D11HullShader*>(shader), nullptr, 0 );
        break;
    case kComputeShader:
        pContext->CSSetShader( reinterpret_cast<ID3D11ComputeShader*>(shader), nullptr, 0 );
        break;
    }
}

ShaderByteCode::ShaderByteCode( const std::string& name, void* pBytecode, size_t length ) :
	pShaderBytecode(pBytecode), Length(length)
{
	Name = Utility::MakeWStr(name);
}

