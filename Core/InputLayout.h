#pragma once

#include "pch.h"

struct ShaderByteCode;

struct InputDesc
{
	enum { kLength = 64 };
	char SemanticName[kLength];
    UINT SemanticIndex;
    DXGI_FORMAT Format;
    UINT InputSlot;
    UINT AlignedByteOffset;
    D3D11_INPUT_CLASSIFICATION InputSlotClass;
    UINT InstanceDataStepRate;
};
std::size_t hash_value( const InputDesc& Desc );

class InputLayout
{
public:

	static std::shared_ptr<InputLayout> Create(const std::vector<InputDesc>& Desc, const ShaderByteCode& VS); 
	static void DestroyAll();
	void Bind( ID3D11DeviceContext* pContext );

private:

	InputLayout();
	void GetInputLayout( const std::vector<InputDesc>& Desc, const ShaderByteCode& Shader );

	ID3D11InputLayout* m_InputLayout;
};

