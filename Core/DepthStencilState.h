#pragma once

struct DepthStencilDesc
{
public:

	DepthStencilDesc() : Desc(D3D11_DEFAULT), StencilRef(0)
	{
	}

    CD3D11_DEPTH_STENCIL_DESC Desc;
	UINT StencilRef;
};

class DepthStencilState
{
public:
	static std::shared_ptr<DepthStencilState> Create(const DepthStencilDesc& desc);
	static void DestroyAll();
	void Bind( ID3D11DeviceContext* pContext );

private:
	DepthStencilState();
	void GetState( const CD3D11_DEPTH_STENCIL_DESC& desc );

	UINT m_StencilRef;
	ID3D11DepthStencilState* m_DepthStencilState;
};
