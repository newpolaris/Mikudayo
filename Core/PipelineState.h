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

class CommandContext;
class GraphicsContext;
class VertexShader;
class GeometryShader;
class HullShader;
class DomainShader;
class PixelShader;
class ComputeShader;
class InputLayout;
class BlendState;
class DepthStencilState;
class RasterizerState;
class Shader;
struct InputDesc;
struct GraphicsPipelineStateDesc;

class PSO
{
public:

	PSO() {}
	virtual ~PSO() {}
};

struct PipelineState
{
    D3D_PRIMITIVE_TOPOLOGY TopologyType;
	std::shared_ptr<InputLayout> InputLayout;
	std::shared_ptr<BlendState> BlendState;
	std::shared_ptr<DepthStencilState> DepthStencilState;
	std::shared_ptr<RasterizerState> RasterizerState;
	std::shared_ptr<Shader> VertexShader;
	std::shared_ptr<Shader> PixelShader;

	void Bind( ID3D11DeviceContext3* Context);
};

class GraphicsPSO : public PSO
{
	friend class GraphicsContext;

public:

	// Start with empty state
	GraphicsPSO();
	GraphicsPSO(const GraphicsPSO& PSO);
	GraphicsPSO& operator=(const GraphicsPSO& PSO);
	virtual ~GraphicsPSO();
	void Destroy();

	std::shared_ptr<PipelineState> GraphicsPSO::GetState();

	void SetBlendState( const D3D11_BLEND_DESC& BlendDesc );
	void SetRasterizerState( const D3D11_RASTERIZER_DESC& RasterizerDesc );
	void SetDepthStencilState( const D3D11_DEPTH_STENCIL_DESC& DepthStencilDesc );
	void SetSampleMask( UINT SampleMask );
	void SetStencilRef( UINT StencilRef );
	void SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY TopologyType );
	void SetRenderTargetFormat( DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0 );
	void SetRenderTargetFormats( UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0 );
	void SetInputLayout( UINT NumElements, const InputDesc* pInputElementDescs );
	void SetVertexShader( const std::string& Name, const void* Binary, size_t Size );
	void SetPixelShader( const std::string& Name, const void* Binary, size_t Size );
	void SetGeometryShader( const std::string& Name, const void* Binary, size_t Size );
	void SetHullShader( const std::string& Name, const void* Binary, size_t Size );
	void SetDomainShader( const std::string& Name, const void* Binary, size_t Size );

	// Perform validation and compute a hash value for fast state block comparisons
	void Finalize();

private:
	std::unique_ptr<GraphicsPipelineStateDesc> m_PSODesc;
	std::promise<void> m_Promise;
	std::shared_future<void> m_ReadyFuture;
	std::shared_ptr<PipelineState> m_PSOState;
};

class ComputePSO : public PSO
{
	friend class CommandContext;

public:
	ComputePSO();
	virtual ~ComputePSO() {}


	// void SetComputeShader( const ShaderByteCode& Binary ) { m_PSODesc.CS = Binary; }

	void Finalize();

private:

	// D3D12_COMPUTE_PIPELINE_STATE_DESC m_PSODesc;
};
