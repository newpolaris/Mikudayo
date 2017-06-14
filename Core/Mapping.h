#pragma once

#include "pch.h"

using D3D11_SRV_HANDLE = ID3D11ShaderResourceView*;
using D3D11_UAV_HANDLE = ID3D11UnorderedAccessView*;
using D3D11_RTV_HANDLE = ID3D11RenderTargetView*;
using D3D11_DSV_HANDLE = ID3D11DepthStencilView*;
using D3D11_SAMPLER_HANDLE = ID3D11SamplerState*;
using D3D11_BUFFER_HANDLE = ID3D11Buffer*;

struct D3D11_VERTEX_BUFFER_VIEW
{
	ID3D11Buffer* Buffer;
	UINT Offset;
	UINT StrideInBytes;
};

struct D3D11_INDEX_BUFFER_VIEW
{
	ID3D11Buffer* Buffer;
    DXGI_FORMAT Format;
	UINT Offset;
};

enum EPipelineBind : uint8_t
{
	kBindVertex = 0,
	kBindHull,
	kBindDomain,
	kBindGeometry,
	kBindPixel,
	kBindCompute,
};

using BindList = std::initializer_list<EPipelineBind>;
