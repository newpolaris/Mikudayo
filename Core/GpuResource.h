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

class GpuResource
{

public:
	GpuResource() { }

	virtual void Destroy()
	{
		ASSERT(m_pResource.Reset() == 0);
		m_pResource = nullptr;
	}

	ID3D11Resource* operator->() { return m_pResource.Get(); } 
	const ID3D11Resource* operator->() const { return m_pResource.Get(); }

	ID3D11Resource* GetResource() { return m_pResource.Get(); } 
	const ID3D11Resource* GetResource() const { return m_pResource.Get(); }

protected:

	Microsoft::WRL::ComPtr<ID3D11Resource> m_pResource;
};
