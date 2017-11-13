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
struct StreamOutDesc;
struct GraphicsPipelineStateDesc;
struct ComputePipelineStateDesc;

struct ComputePipelineState
{
	std::shared_ptr<Shader> ComputeShader;
	void Bind( ID3D11DeviceContext* Context);
};

struct GraphicsPipelineState
{
    D3D_PRIMITIVE_TOPOLOGY TopologyType;
	std::shared_ptr<InputLayout> InputLayout;
	std::shared_ptr<BlendState> BlendState;
	std::shared_ptr<DepthStencilState> DepthStencilState;
	std::shared_ptr<RasterizerState> RasterizerState;
	std::shared_ptr<Shader> VertexShader;
	std::shared_ptr<Shader> PixelShader;
	std::shared_ptr<Shader> GeometryShader;
	std::shared_ptr<Shader> DomainShader;
	std::shared_ptr<Shader> HullShader;

	void Bind( ID3D11DeviceContext* Context);
};

class PSO
{
public:
    enum ELoadingState : uint8_t {
        kStateUnloaded,
        kStateLoading,
        kStateLoaded,
    };

	PSO() : m_LoadingState(kStateUnloaded) {}
    static void DestroyAll( void );
	virtual ~PSO() {}

    std::atomic<ELoadingState> m_LoadingState;
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

	GraphicsPipelineState* GetState();

    D3D11_RASTERIZER_DESC GetRasterizerState() const;

	void SetBlendState( const D3D11_BLEND_DESC& BlendDesc );
	void SetRasterizerState( const D3D11_RASTERIZER_DESC& RasterizerDesc );
	void SetDepthStencilState( const D3D11_DEPTH_STENCIL_DESC& DepthStencilDesc );
    void SetBlendFactor( FLOAT BlendFactor[4] );
	void SetSampleMask( UINT SampleMask );
	void SetStencilRef( UINT StencilRef );
	void SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY TopologyType );
	void SetRenderTargetFormat( DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0 );
	void SetRenderTargetFormats( UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0 );
	void SetInputLayout( UINT NumElements, const InputDesc* pInputElementDescs );
    void SetStreamOutEntries( UINT NumElements, const StreamOutDesc* pStreamoutDescs );
	void SetVertexShader( const std::string& Name, const void* Binary, size_t Size );
	void SetPixelShader( const std::string& Name, const void* Binary, size_t Size );
	void SetGeometryShader( const std::string& Name, const void* Binary, size_t Size );
	void SetHullShader( const std::string& Name, const void* Binary, size_t Size );
	void SetDomainShader( const std::string& Name, const void* Binary, size_t Size );

	// Perform validation
	void Finalize();

private:
	std::unique_ptr<GraphicsPipelineStateDesc> m_PSODesc;
	GraphicsPipelineState* m_PSOState;
};

class ComputePSO : public PSO
{
	friend class CommandContext;

public:
	ComputePSO();
	ComputePSO(const ComputePSO& PSO);
	ComputePSO& operator=(const ComputePSO& PSO);
    virtual ~ComputePSO();
	void Destroy();

	ComputePipelineState* GetState();

	void SetComputeShader( const std::string& Name, const void* Binary, size_t Size );
	void Finalize();

private:

	std::unique_ptr<ComputePipelineStateDesc> m_PSODesc;
	ComputePipelineState* m_PSOState;
};
