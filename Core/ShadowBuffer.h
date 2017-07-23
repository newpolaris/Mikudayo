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
    ShadowBuffer() : m_DepthFormat(DXGI_FORMAT_D16_UNORM) {}
        
    void Create( const std::wstring& Name, uint32_t Width, uint32_t Height );
    void Create( const std::wstring& Name, uint32_t Width, uint32_t Height, EsramAllocator& Allocator );

    D3D11_SRV_HANDLE GetSRV() const { return GetDepthSRV(); }

    void BeginRendering( GraphicsContext& context );
    void EndRendering( GraphicsContext& context );
    void SetDepthFormat( DXGI_FORMAT Format );

private:
    D3D11_VIEWPORT m_Viewport;
    D3D11_RECT m_Scissor;
    DXGI_FORMAT m_DepthFormat;
};

inline void ShadowBuffer::SetDepthFormat( DXGI_FORMAT Format )
{
    m_DepthFormat = Format;
}
