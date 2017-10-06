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

#include "pch.h"
#include "PipelineState.h"
#include "Hash.h"
#include "GraphicsCore.h"
#include "Shader.h"
#include "BlendState.h"
#include "DepthStencilState.h"
#include "RasterizerState.h"
#include "InputLayout.h"
#include "Hash.h"
#include <map>
#include <thread>

using Microsoft::WRL::ComPtr;
using Graphics::g_Device;
using namespace std;

static map< size_t, std::shared_ptr<GraphicsPipelineState>> s_GraphicsPSOHashMap;
static map< size_t, std::shared_ptr<ComputePipelineState>> s_ComputePSOHashMap;

struct ComputePipelineStateDesc
{
public:
    ComputePipelineStateDesc() {}
    size_t Hash() const;
    ShaderByteCode CS;
};

struct GraphicsPipelineStateDesc
{
public:
	GraphicsPipelineStateDesc() : TopologyType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {}
    size_t Hash() const;

    D3D_PRIMITIVE_TOPOLOGY TopologyType;
	std::vector<InputDesc> InputDescList;
    ShaderByteCode VS;
    ShaderByteCode PS;
    ShaderByteCode DS;
    ShaderByteCode HS;
    ShaderByteCode GS;
	BlendDesc Blend;
	DepthStencilDesc DepthStencil;
	RasterizerDesc Rasterizer;
};

void PSO::DestroyAll(void)
{
    s_GraphicsPSOHashMap.clear();
    s_ComputePSOHashMap.clear();
}

ComputePSO::ComputePSO() : m_PSOState(nullptr)
{
	m_PSODesc = std::make_unique<ComputePipelineStateDesc>();
}

GraphicsPSO::GraphicsPSO() : m_PSOState(nullptr)
{
	m_PSODesc = std::make_unique<GraphicsPipelineStateDesc>();
}

ComputePSO::ComputePSO( const ComputePSO& PSO )
{
	m_PSODesc = std::make_unique<ComputePipelineStateDesc>(*PSO.m_PSODesc);
}

GraphicsPSO::GraphicsPSO( const GraphicsPSO& PSO )
{
	m_PSODesc = std::make_unique<GraphicsPipelineStateDesc>(*PSO.m_PSODesc);
}

ComputePSO& ComputePSO::operator=( const ComputePSO& PSO )
{
	m_PSODesc = std::make_unique<ComputePipelineStateDesc>(*PSO.m_PSODesc);
	return *this;
}

GraphicsPSO& GraphicsPSO::operator=( const GraphicsPSO& PSO )
{
	m_PSODesc = std::make_unique<GraphicsPipelineStateDesc>(*PSO.m_PSODesc);
	return *this;
}

ComputePSO::~ComputePSO()
{
    Destroy();
}

GraphicsPSO::~GraphicsPSO()
{
    Destroy();
}

void ComputePSO::Destroy()
{
    while (m_LoadingState == kStateLoading)
        std::this_thread::yield();
	m_PSOState = nullptr;
}

void GraphicsPSO::Destroy()
{
    while (m_LoadingState == kStateLoading)
        std::this_thread::yield();
	m_PSOState = nullptr;
}

ComputePipelineState* ComputePSO::GetState()
{
	ASSERT( m_LoadingState != kStateUnloaded, L"Not Finalized Yet" );
    while (m_LoadingState == kStateLoading)
        std::this_thread::yield();

	return m_PSOState;
}

GraphicsPipelineState* GraphicsPSO::GetState()
{
	ASSERT( m_LoadingState != kStateUnloaded, L"Not Finalized Yet" );
    while (m_LoadingState == kStateLoading)
        std::this_thread::yield();

	return m_PSOState;
}

void GraphicsPSO::SetBlendState( const D3D11_BLEND_DESC& BlendDesc )
{
	m_PSODesc->Blend.Desc = CD3D11_BLEND_DESC( BlendDesc );
}

void GraphicsPSO::SetRasterizerState( const D3D11_RASTERIZER_DESC& RasterizerDesc )
{
	m_PSODesc->Rasterizer.Desc = CD3D11_RASTERIZER_DESC(RasterizerDesc);
}

void GraphicsPSO::SetDepthStencilState( const D3D11_DEPTH_STENCIL_DESC& DepthStencilDesc )
{
	m_PSODesc->DepthStencil.Desc = CD3D11_DEPTH_STENCIL_DESC(DepthStencilDesc);
}

void GraphicsPSO::SetBlendFactor( FLOAT BlendFactor[4] )
{
    for (int i = 0; i < _countof(m_PSODesc->Blend.BlendFactor); i++)
        m_PSODesc->Blend.BlendFactor[i] = BlendFactor[i];
}

void GraphicsPSO::SetSampleMask( UINT SampleMask )
{
	m_PSODesc->Blend.SampleMask = SampleMask;
}

void GraphicsPSO::SetStencilRef( UINT StencilRef )
{
	m_PSODesc->DepthStencil.StencilRef = StencilRef;
}

void GraphicsPSO::SetPrimitiveTopologyType( D3D11_PRIMITIVE_TOPOLOGY TopologyType )
{
	ASSERT(TopologyType != D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED, "Can't draw with undefined topology");
	m_PSODesc->TopologyType = TopologyType;
}

void GraphicsPSO::SetRenderTargetFormat( DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality )
{
    (RTVFormat), (DSVFormat), (MsaaCount), (MsaaQuality);
}

void GraphicsPSO::SetRenderTargetFormats( UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality )
{
    (NumRTVs), (RTVFormats), (DSVFormat), (MsaaCount), (MsaaQuality);
}

void GraphicsPSO::SetInputLayout( UINT NumElements, const InputDesc* pInputElementDescs )
{
	if (NumElements > 0)
	{
		ASSERT( pInputElementDescs != nullptr );
		std::vector<InputDesc> Desc;
		std::copy(pInputElementDescs, pInputElementDescs + NumElements, std::back_inserter(Desc));
		m_PSODesc->InputDescList.swap(Desc);
	}
	else
	{
		m_PSODesc->InputDescList.clear();
	}
}

void GraphicsPSO::SetVertexShader( const std::string& Name, const void* Binary, size_t Size )
{
	m_PSODesc->VS = ShaderByteCode { Name, const_cast<void*>(Binary), Size };
}

void GraphicsPSO::SetPixelShader( const std::string& Name, const void* Binary, size_t Size )
{
	m_PSODesc->PS = ShaderByteCode { Name, const_cast<void*>(Binary), Size };
}

void GraphicsPSO::SetGeometryShader( const std::string& Name, const void * Binary, size_t Size )
{
	m_PSODesc->GS = ShaderByteCode { Name, const_cast<void*>(Binary), Size };
}

void GraphicsPSO::SetHullShader( const std::string & Name, const void * Binary, size_t Size )
{
	m_PSODesc->HS = ShaderByteCode { Name, const_cast<void*>(Binary), Size };
}

void GraphicsPSO::SetDomainShader( const std::string & Name, const void * Binary, size_t Size )
{
	m_PSODesc->DS = ShaderByteCode { Name, const_cast<void*>(Binary), Size };
}

void ComputePSO::SetComputeShader( const std::string & Name, const void* Binary, size_t Size )
{
	m_PSODesc->CS = ShaderByteCode { Name, const_cast<void*>(Binary), Size };
}

void GraphicsPSO::Finalize()
{
	ASSERT(m_LoadingState != kStateLoaded, L"Already Finalized");
    m_LoadingState.store( kStateLoading );

    static mutex s_HashMapMutex;
	auto sync = std::async(std::launch::async, [=]{
        const size_t HashCode = m_PSODesc->Hash();
        lock_guard<mutex> CS(s_HashMapMutex);
        auto iter = s_GraphicsPSOHashMap.find(HashCode);
        if (iter == s_GraphicsPSOHashMap.end())
        {
            auto State = std::make_shared<GraphicsPipelineState>();
            State->TopologyType = m_PSODesc->TopologyType;
            State->InputLayout = InputLayout::Create( m_PSODesc->InputDescList, m_PSODesc->VS );
            State->BlendState = BlendState::Create( m_PSODesc->Blend );
            State->DepthStencilState = DepthStencilState::Create( m_PSODesc->DepthStencil );
            State->RasterizerState = RasterizerState::Create( m_PSODesc->Rasterizer );
            State->VertexShader = Shader::Create( kVertexShader, m_PSODesc->VS );
            State->PixelShader = Shader::Create( kPixelShader, m_PSODesc->PS );
            State->GeometryShader = Shader::Create( kGeometryShader, m_PSODesc->GS );
            State->DomainShader = Shader::Create( kDomainShader, m_PSODesc->DS );
            State->HullShader = Shader::Create( kDomainShader, m_PSODesc->HS );
            s_GraphicsPSOHashMap[HashCode] = State;
            m_PSOState = State.get();
        }
        else
        {
            m_PSOState = iter->second.get();
        }
        m_LoadingState.store( kStateLoaded );
	});
}

void ComputePSO::Finalize()
{
	ASSERT(m_LoadingState != kStateLoaded, L"Already Finalized");

    m_LoadingState.store( kStateLoading );
    static mutex s_HashMapMutex;
	auto sync = std::async(std::launch::async, [=]{
        size_t HashCode = m_PSODesc->Hash();
        lock_guard<mutex> CS(s_HashMapMutex);
        auto iter = s_ComputePSOHashMap.find(HashCode);
        if (iter == s_ComputePSOHashMap.end())
        {
            auto State = std::make_shared<ComputePipelineState>();
            State->ComputeShader = Shader::Create( kComputeShader, m_PSODesc->CS );
            s_ComputePSOHashMap[HashCode] = State;
            m_PSOState = State.get();
        }
        else
        {
            m_PSOState = iter->second.get();
        }
        m_LoadingState.store( kStateLoaded );
	});
}

void GraphicsPipelineState::Bind( ID3D11DeviceContext* Context )
{
	if (TopologyType != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
		Context->IASetPrimitiveTopology( TopologyType );

	if (InputLayout)
		InputLayout->Bind( Context );

    VertexShader->Bind( Context );
    PixelShader->Bind( Context );
    GeometryShader->Bind( Context );
    DomainShader->Bind( Context );
    HullShader->Bind( Context );

	if (BlendState)
		BlendState->Bind( Context );

	if (DepthStencilState)
		DepthStencilState->Bind( Context );

	if (RasterizerState)
		RasterizerState->Bind( Context );
}

void ComputePipelineState::Bind( ID3D11DeviceContext * Context )
{
	if (ComputeShader)
		ComputeShader->Bind( Context );
}

size_t GraphicsPipelineStateDesc::Hash() const
{
    size_t HashCode = Utility::HashState(&TopologyType);
    HashCode = Utility::HashState(InputDescList.data(), InputDescList.size(), HashCode);
    HashCode = Utility::HashState(&VS, 1, HashCode);
    HashCode = Utility::HashState(&PS, 1, HashCode);
    HashCode = Utility::HashState(&DS, 1, HashCode);
    HashCode = Utility::HashState(&HS, 1, HashCode);
    HashCode = Utility::HashState(&GS, 1, HashCode);
    HashCode = Utility::HashState(&Blend, 1, HashCode);
    HashCode = Utility::HashState(&DepthStencil, 1, HashCode);
    HashCode = Utility::HashState(&Rasterizer, 1, HashCode);
    return HashCode;
}

size_t ComputePipelineStateDesc::Hash() const
{
    return Utility::HashState(&CS);
}
