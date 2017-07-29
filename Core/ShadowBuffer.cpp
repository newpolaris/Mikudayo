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
#include "ShadowBuffer.h"
#include "EsramAllocator.h"
#include "CommandContext.h"

void ShadowBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height )
{
    DepthBuffer::Create( Name, Width, Height, m_DepthFormat );
    SetViewport( Width, Height );
}

void ShadowBuffer::Create( const std::wstring& Name, uint32_t Width, uint32_t Height, EsramAllocator& Allocator )
{
    DepthBuffer::Create( Name, Width, Height, m_DepthFormat, Allocator );
    SetViewport( Width, Height );
}

void ShadowBuffer::CreateArray( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArraySize )
{
    DepthBuffer::CreateArray( Name, Width, Height, ArraySize, m_DepthFormat );
    SetViewport( Width, Height );
}

void ShadowBuffer::CreateArray( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArraySize, EsramAllocator& Allocator )
{
    DepthBuffer::CreateArray( Name, Width, Height, ArraySize, m_DepthFormat, Allocator );
    SetViewport( Width, Height );
}

void ShadowBuffer::SetViewport( uint32_t Width, uint32_t Height )
{
    m_Viewport.TopLeftX = 0.0f;
    m_Viewport.TopLeftY = 0.0f;
    m_Viewport.Width = (float)Width;
    m_Viewport.Height = (float)Height;
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    // Prevent drawing to the boundary pixels so that we don't have to worry about shadows stretching
    m_Scissor.left = 1;
    m_Scissor.top = 1;
    m_Scissor.right = (LONG)Width - 2;
    m_Scissor.bottom = (LONG)Height - 2;
}

void ShadowBuffer::BeginRendering( GraphicsContext& Context )
{
    Context.ClearDepth(*this);
    Context.SetDepthStencilTarget(GetDSV());
    Context.SetViewportAndScissor(m_Viewport, m_Scissor);
}

void ShadowBuffer::BeginRendering( GraphicsContext& Context, uint32_t Slice )
{
    Context.ClearDepth(*this, Slice);
    Context.SetDepthStencilTarget(GetDSV(Slice));
    Context.SetViewportAndScissor(m_Viewport, m_Scissor);
}

void ShadowBuffer::EndRendering( GraphicsContext& Context )
{
    Context.SetDepthStencilTarget(nullptr);
}
