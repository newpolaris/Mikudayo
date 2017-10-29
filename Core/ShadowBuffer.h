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

#include "DepthBuffer.h"

class EsramAllocator;

class GraphicsContext;

class ShadowBuffer : public DepthBuffer
{
public:
    ShadowBuffer() : m_DepthFormat(DXGI_FORMAT_D32_FLOAT) {}

    void Create( const std::wstring& Name, uint32_t Width, uint32_t Height );
    void Create( const std::wstring& Name, uint32_t Width, uint32_t Height, EsramAllocator& Allocator );
    void CreateArray( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArraySize );
    void CreateArray( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArraySize, EsramAllocator& Allocator );

    D3D11_SRV_HANDLE GetSRV() const { return GetDepthSRV(); }
    D3D11_SRV_HANDLE GetSRV( uint32_t slice ) const { return GetDepthSRV(slice); }

    void BeginRendering( GraphicsContext& Context );
    void BeginRendering( GraphicsContext& Context, uint32_t Slice );
    void EndRendering( GraphicsContext& context );
    void SetDepthFormat( DXGI_FORMAT Format );

private:
    void SetViewport( uint32_t Width, uint32_t Height );

    D3D11_VIEWPORT m_Viewport;
    D3D11_RECT m_Scissor;
    DXGI_FORMAT m_DepthFormat;
};

inline void ShadowBuffer::SetDepthFormat( DXGI_FORMAT Format )
{
    m_DepthFormat = Format;
}
