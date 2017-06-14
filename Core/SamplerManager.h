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
// Author:  James Stanard
//

#pragma once

#include "pch.h"
#include "Color.h"
#include "Mapping.h"

class SamplerDesc : public D3D11_SAMPLER_DESC
{
public:
	static void DestroyAll();

	SamplerDesc()
	{
		Filter = D3D11_FILTER_ANISOTROPIC;
		AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		MipLODBias = 0.0f;
		MaxAnisotropy = 16;
		ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
		BorderColor[0] = 1.0f;
		BorderColor[1] = 1.0f;
		BorderColor[2] = 1.0f;
		BorderColor[3] = 1.0f;
		MinLOD = 0.0f;
		MaxLOD = D3D11_FLOAT32_MAX;
	}

	void SetTextureAddressMode( D3D11_TEXTURE_ADDRESS_MODE AddressMode )
	{
		AddressU = AddressMode;
		AddressV = AddressMode;
		AddressW = AddressMode;
	}

	void SetBorderColor( Color Border )
	{
		BorderColor[0] = Border.R();
		BorderColor[1] = Border.G();
		BorderColor[2] = Border.B();
		BorderColor[3] = Border.A();
	}

	// Allocate new descriptor as needed; return handle to existing descriptor when possible
	D3D11_SAMPLER_HANDLE CreateDescriptor( void );

	D3D11_SAMPLER_HANDLE m_SamplerState;
};
