#pragma once

struct BlendDesc
{
public:

	BlendDesc() : Desc(D3D11_DEFAULT), SampleMask(0xFFFFFFFF) 
	{
		for (UINT i = 0; i < 4; i++)
			BlendFactor[i] = 1.0f;
	}

    CD3D11_BLEND_DESC Desc;
	FLOAT BlendFactor[4];
	UINT SampleMask;
};

class BlendState
{
public:

	static std::shared_ptr<BlendState> Create(const BlendDesc& desc); 
	static void DestroyAll();
	void Bind( ID3D11DeviceContext* pContext );

private:

	BlendState();
	void GetBlendState( const CD3D11_BLEND_DESC& desc );

	FLOAT m_BlendFactor[4];
	UINT m_SampleMask;
	ID3D11BlendState* m_BlendState;
};

