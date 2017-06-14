//--------------------------------------------------------------------------------
// This file is a portion of the Hieroglyph 3 Rendering Engine.  It is distributed
// under the MIT License, available in the root of this distribution and 
// at the following URL:
//
// http://www.opensource.org/licenses/mit-license.php
//
// Copyright (c) Jason Zink 
//--------------------------------------------------------------------------------

#pragma once

#include "pch.h"

enum ShaderType : uint8_t
{
	kVertexShader = 0,
	kHullShader,
	kDomainShader,
	kGeometryShader,
	kPixelShader, 
	kComputeShader,
	kShaderCount,
};

struct ShaderInputBindDesc
{
	ShaderInputBindDesc(D3D11_SHADER_INPUT_BIND_DESC desc)
	{
		Name = std::string(desc.Name);
		Type = desc.Type;
		BindPoint = desc.BindPoint;
		BindCount = desc.BindCount;
		uFlags = desc.uFlags;
		ReturnType = desc.ReturnType;
		Dimension = desc.Dimension;
		NumSamples = desc.NumSamples;
	};

	ShaderInputBindDesc()
	{
		Type = D3D_SIT_CBUFFER;
		BindPoint = 0;
		BindCount = 0;
		uFlags = 0;
		ReturnType = D3D11_RETURN_TYPE_UNORM;
		Dimension = D3D_SRV_DIMENSION_UNKNOWN;
		NumSamples = 0;
	};

	std::string Name;
	D3D_SHADER_INPUT_TYPE Type;
	UINT BindPoint;
	UINT BindCount;
	UINT uFlags;
	D3D11_RESOURCE_RETURN_TYPE ReturnType;
	D3D_SRV_DIMENSION Dimension;
	UINT NumSamples;
};

struct ConstantBufferLayout
{
	D3D11_SHADER_BUFFER_DESC Description;
	std::vector<D3D11_SHADER_VARIABLE_DESC>	Variables;
	std::vector<D3D11_SHADER_TYPE_DESC> Types;
};

struct ShaderByteCode
{
public:
	ShaderByteCode() : pShaderBytecode(nullptr), Length(0) {}
	ShaderByteCode( const std::string& name, void* pBytecode, size_t length );
	bool valid() const
	{
		if (pShaderBytecode == nullptr && Length == 0)
			return false;
		ASSERT(pShaderBytecode != nullptr && Length != 0);
		return true;
	}

	std::wstring Name;
	void* pShaderBytecode;
	size_t Length;
};

class Shader
{
public:
	static std::shared_ptr<Shader> Create( ShaderType Type, const ShaderByteCode& ByteCode ); 
	static void DestroyAll();
	bool ShaderCheckResource( D3D_SHADER_INPUT_TYPE inputType, UINT slot, std::string name );
	void Bind( ID3D11DeviceContext* pContext );
	~Shader();

private:
	Shader(ShaderType Type);
	void Create( const ShaderByteCode& ByteCode );
	void FillReflection();

	ShaderType m_ShaderType;
	std::string m_Name;
	ShaderByteCode m_ShaderByteCode;

	Microsoft::WRL::ComPtr<ID3DBlob> m_Blob;
	Microsoft::WRL::ComPtr<ID3D11DeviceChild> m_Shader;

	D3D11_SHADER_DESC m_ShaderDescription;
	std::vector<D3D11_SIGNATURE_PARAMETER_DESC> m_InputSignatureParameters;
	std::vector<D3D11_SIGNATURE_PARAMETER_DESC> m_OutputSignatureParameters;
	std::vector<ConstantBufferLayout> m_BufferDescription;
	std::vector<ShaderInputBindDesc> m_ResourceDescrition;
};