#pragma once

#include "pch.h"
#include "CommandListManager.h"
#include "GraphicsCore.h"
#include "PipelineState.h"
#include "Mapping.h"
#include "ConstantBuffer.h"

class ColorBuffer;
class DepthBuffer;
class Texture;
class GraphicsContext;
class ComputeContext;
class Color;
class GpuBuffer;

struct DWParam
{
	DWParam( FLOAT f ) : Float(f) {}
	DWParam( UINT u ) : Uint(u) {}
	DWParam( INT i ) : Int(i) {}

	void operator= ( FLOAT f ) { Float = f; }
	void operator= ( UINT u ) { Uint = u; }
	void operator= ( INT i ) { Int = i; }

	union
	{
		FLOAT Float;
		UINT Uint;
		INT Int;
	};
};

enum ContextType : uint8_t
{
	kComputeContext = 0,
	kGraphicsContext,
	kContextCount,
};

class ContextManager
{
public:
	ContextManager(void) {}

	CommandContext* AllocateContext(ContextType Type);
	void FreeContext(CommandContext*);
	void DestroyAllContexts();

private:
	std::vector<std::unique_ptr<CommandContext> > sm_ContextPool;
	std::queue<CommandContext*> sm_AvailableContexts[ContextType::kContextCount];
	std::mutex sm_ContextAllocationMutex;
};

struct DynAlloc
{
	DynAlloc( D3D11_BUFFER_HANDLE handle, UINT firstConstant, UINT numConstants )
	{
		Handle = handle;
		FirstConstant = firstConstant;
		NumConstants = numConstants;
	}
	void* DataPtr;
	D3D11_BUFFER_HANDLE Handle;
	UINT FirstConstant;
	UINT NumConstants;
};

// Constant blocks must be multiples of 16 constants @ 16 bytes each
#define DEFAULT_ALIGN 256

class LinearAllocator
{
public:
	const size_t InitSize = 0x800000;	// 8 MB

	LinearAllocator() : m_Context(nullptr) 
	{
		m_PageSize = InitSize;
	}

	~LinearAllocator() { m_Context = nullptr; }
	void Initialize( ID3D11DeviceContext3* Context ) 
	{
		m_Context = Context;
	}
	void Reset();

	DynAlloc Allocate( size_t SizeInBytes, size_t Alignment = DEFAULT_ALIGN );

	void CreatePage();

	void Finish()
	{
		if (m_Buffer) 
		{
			m_Context->Unmap( m_Buffer.Get(), 0 );
			m_CurOffset = 0;
			m_AllocPointer = nullptr;
		}
	}

	ID3D11DeviceContext3* m_Context;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer;
	size_t m_PageSize;
	size_t m_CurOffset;
	uint8_t* m_AllocPointer;
};


struct NonCopyable
{
	NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable & operator=(const NonCopyable&) = delete;
};

class CommandContext : NonCopyable
{
	friend ContextManager;

protected:
	CommandContext();

private:
	void Reset( void );

public:
	enum { kNumConstant = 8 };

	virtual ~CommandContext(void);

	static void DestroyAllContexts(void);

	static CommandContext& Begin( ContextType Type, const std::wstring ID = L"" );

	// Flush existing commands to the GPU but keep the context alive
	uint64_t Flush( bool WaitForCompletion = false );

	// Flush existing commands and release the current context
	uint64_t Finish( bool WaitForCompletion = false );

	// Prepare to render by reserving a command list and command allocator
	void Initialize( void );
	template <typename T>
	void SetDynamicConstantBufferView( UINT Slot, const ConstantBuffer<T>& Buffer, BindList BindList );
	void SetDynamicConstantBufferView( UINT Slot, size_t BufferSize, const void* BufferData, BindList Binds );
	void SetDynamicDescriptor( UINT Offset, const D3D11_SRV_HANDLE Handle, BindList Binds );
	void SetDynamicDescriptors( UINT Offset, UINT Count, const D3D11_SRV_HANDLE Handles[], BindList Binds );
	void SetDynamicSampler( UINT Offset, const D3D11_SAMPLER_HANDLE Handle, EPipelineBind Bind );
	void SetDynamicSamplers( UINT Offset, UINT Count, const D3D11_SAMPLER_HANDLE Handle[], BindList BindList );
	void SetConstantBuffers( UINT Offset, UINT Count, const D3D11_BUFFER_HANDLE Handle[], BindList BindList );
	void UploadContstantBuffer( D3D11_BUFFER_HANDLE Handle, void const* Data, size_t Size );
	void SetConstants( UINT NumConstants, const void* pConstants, BindList BindList );
	void SetConstants( DWParam X, BindList BindList );
	void SetConstants( DWParam X, DWParam Y, BindList BindList );
	void SetConstants( DWParam X, DWParam Y, DWParam Z, BindList BindList );
	void SetConstants( DWParam X, DWParam Y, DWParam Z, DWParam W, BindList BindList );

	GraphicsContext& GetGraphicsContext() {
		return reinterpret_cast<GraphicsContext&>(*this);
	}

	void PIXBeginEvent(const wchar_t* label);
	void PIXEndEvent(void);
	void PIXSetMarker(const wchar_t* label);

protected:
	CommandListManager* m_OwningManager;
	ID3D11DeviceContext3* m_Context;

	std::wstring m_ID;
	void SetID(const std::wstring& ID) { m_ID = ID; }

	ContextType m_Type;
	
	__declspec(align(16)) struct InternalCBStorage {
		float v[kNumConstant];
	};
	ConstantBuffer<InternalCBStorage> m_InternalCB;
	LinearAllocator m_CpuLinearAllocator;
};

class ComputeContext : public CommandContext
{
};

class GraphicsContext : public CommandContext
{
	friend ContextManager;

protected:

	GraphicsContext();

public:

	static GraphicsContext& Begin(const std::wstring& ID = L"")
	{
		return CommandContext::Begin(ContextType::kGraphicsContext, ID).GetGraphicsContext();
	}

	void SetRenderTargets( UINT NumRTVs, const D3D11_RTV_HANDLE RTVs[] );
	void SetRenderTargets( UINT NumRTVs, const D3D11_RTV_HANDLE RTVs[], D3D11_DSV_HANDLE DSV );
	void SetRenderTarget( D3D11_RTV_HANDLE RTV ) { SetRenderTargets( 1, &RTV ); }
	void SetRenderTarget( D3D11_RTV_HANDLE RTV, D3D11_DSV_HANDLE DSV );
	void SetDepthStencilTarget( D3D11_DSV_HANDLE DSV ) { SetRenderTargets(0, nullptr, DSV); }

	void GenerateMips( D3D11_SRV_HANDLE SRV );
	void SetViewport( const D3D11_VIEWPORT& vp );
	void SetViewport( FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f );
	void SetScissor( const D3D11_RECT& rect );
	void SetScissor( UINT left, UINT top, UINT right, UINT bottom );
	void SetViewportAndScissor( const D3D11_VIEWPORT& vp, const D3D11_RECT& rect );
	void SetViewportAndScissor( UINT x, UINT y, UINT w, UINT h );
	void SetStencilRef( UINT StencilRef );
	void SetBlendFactor( const Color& BlendFactor );
	void SetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY Topology );
	void ClearColor( ColorBuffer& Target );
	void ClearDepth( DepthBuffer& Target );
	void ClearStencil( DepthBuffer& Target );
	void ClearDepthAndStencil( DepthBuffer& Target );
	void SetIndexBuffer( const D3D11_INDEX_BUFFER_VIEW& Buffer );
	void SetVertexBuffer( UINT Slot, const D3D11_VERTEX_BUFFER_VIEW& Buffer );
	void Draw( UINT VertexCount, UINT VertexStartOffset = 0 );
	void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0);
	void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
		UINT StartVertexLocation = 0, UINT StartInstanceLocation = 0);
	void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
		INT BaseVertexLocation, UINT StartInstanceLocation);
	void DrawIndirect( GpuBuffer& ArgumentBuffer, size_t ArgumentBufferOffset = 0 );
	void SetPipelineState( GraphicsPSO& PSO );

	std::shared_ptr<PipelineState> m_PSOState;
};

template<typename T>
void GraphicsContext::SetDynamicConstantBufferView( UINT Slot, const ConstantBuffer<T>& Buffer, BindList BindList )
{
	Buffer.UploadAndBind( *this, Slot, BindList );
}

inline void GraphicsContext::SetViewportAndScissor( UINT x, UINT y, UINT w, UINT h )
{
	SetViewport((float)x, (float)y, (float)w, (float)h);
	SetScissor(x, y, x + w, y + h);
}

inline void GraphicsContext::SetScissor( UINT left, UINT top, UINT right, UINT bottom )
{
	SetScissor(CD3D11_RECT(left, top, right, bottom));
}

inline void CommandContext::PIXBeginEvent(const wchar_t* label)
{
#if defined(RELEASE) && !defined(_PIX_H_)
	(label);
#else
	::PIXBeginEvent(m_Context, 0, label);
#endif
}

inline void CommandContext::PIXEndEvent(void)
{
#if !defined(RELEASE) && !defined(_PIX_H_)
	::PIXEndEvent(m_Context);
#endif
}

inline void CommandContext::PIXSetMarker(const wchar_t* label)
{
#if defined(RELEASE) || _MSC_VER < 1800
	(label);
#else
	::PIXSetMarker(m_Context, 0, label);
#endif
}

inline void GraphicsContext::Draw( UINT VertexCount, UINT VertexStartOffset )
{
	m_Context->Draw( VertexCount, VertexStartOffset );
}

inline void GraphicsContext::DrawIndexed( UINT IndexCount, UINT StartIndexLocation , INT BaseVertexLocation )
{
	m_Context->DrawIndexed( IndexCount, StartIndexLocation, BaseVertexLocation );
}

inline void GraphicsContext::DrawInstanced( UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation )
{
	m_Context->DrawInstanced( VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation );
}

