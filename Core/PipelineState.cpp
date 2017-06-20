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
#include <map>
#include <thread>
#include <mutex>

using Microsoft::WRL::ComPtr;
using Graphics::g_Device;
using namespace std;

struct GraphicsPipelineStateDesc
{
public:

	GraphicsPipelineStateDesc() : TopologyType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {}

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

GraphicsPSO::GraphicsPSO()
{ 
	m_PSODesc = std::make_unique<GraphicsPipelineStateDesc>();
}

GraphicsPSO::GraphicsPSO( const GraphicsPSO& PSO )
{
	m_PSODesc = std::make_unique<GraphicsPipelineStateDesc>(*PSO.m_PSODesc);
}

GraphicsPSO& GraphicsPSO::operator=( const GraphicsPSO& PSO )
{
	m_PSODesc = std::make_unique<GraphicsPipelineStateDesc>(*PSO.m_PSODesc);

	return *this;
}

GraphicsPSO::~GraphicsPSO()
{
	Destroy();
}

void GraphicsPSO::Destroy()
{
	if (m_ReadyFuture.valid())
		m_ReadyFuture.wait();
	m_PSOState = nullptr;
}

std::shared_ptr<PipelineState> GraphicsPSO::GetState() 
{
	ASSERT( m_ReadyFuture.valid(), L"Maybe not finalized yet" );
	m_ReadyFuture.wait();

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

void GraphicsPSO::SetVertexShader( const std::string& Name, const void * Binary, size_t Size )
{
	m_PSODesc->VS = ShaderByteCode { Name, const_cast<void*>(Binary), Size };
}

void GraphicsPSO::SetPixelShader( const std::string & Name, const void * Binary, size_t Size )
{
	m_PSODesc->PS = ShaderByteCode { Name, const_cast<void*>(Binary), Size };
}

void GraphicsPSO::Finalize()
{
	ASSERT(!m_ReadyFuture.valid(), L"Finalized alreay");

	m_ReadyFuture = std::shared_future<void>(m_Promise.get_future());
	auto sync = std::async(std::launch::async, [=]{
		auto State = std::make_shared<PipelineState>();
		State->TopologyType = m_PSODesc->TopologyType;
		State->InputLayout = InputLayout::Create( m_PSODesc->InputDescList, m_PSODesc->VS );
		State->BlendState = BlendState::Create( m_PSODesc->Blend );
		State->DepthStencilState = DepthStencilState::Create( m_PSODesc->DepthStencil );
		State->RasterizerState = RasterizerState::Create( m_PSODesc->Rasterizer );
		State->VertexShader = Shader::Create( kVertexShader, m_PSODesc->VS );
		State->PixelShader = Shader::Create( kPixelShader, m_PSODesc->PS );

		m_PSOState.swap(State);

		m_Promise.set_value();
	});
}

ComputePSO::ComputePSO()
{
}

void PipelineState::Bind( ID3D11DeviceContext3 * Context )
{
	if (TopologyType != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
		Context->IASetPrimitiveTopology( TopologyType );

	if (InputLayout)
		InputLayout->Bind( Context );

	if (VertexShader)
		VertexShader->Bind( Context );

	if (PixelShader)
		PixelShader->Bind( Context );

	if (BlendState)
		BlendState->Bind( Context );

	if (DepthStencilState)
		DepthStencilState->Bind( Context );

	if (RasterizerState)
		RasterizerState->Bind( Context );
}
