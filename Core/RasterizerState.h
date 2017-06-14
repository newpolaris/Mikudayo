#pragma once

class RasterizerDesc
{
public:

	RasterizerDesc() : Desc(D3D11_DEFAULT)
	{
	}

	CD3D11_RASTERIZER_DESC Desc;
};

class RasterizerState
{
public:
	static std::shared_ptr<RasterizerState> Create(const RasterizerDesc& desc); 
	static void DestroyAll();
	void Bind( ID3D11DeviceContext* pContext );

private:
	RasterizerState();
	void GetState( const CD3D11_RASTERIZER_DESC& desc );

	UINT m_StencilRef;
	ID3D11RasterizerState* m_RasterizerState;
};
