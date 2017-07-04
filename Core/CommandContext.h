#pragma once

#include "pch.h"
#include "CommandListManager.h"
#include "GraphicsCore.h"
#include "PipelineState.h"
#include "Mapping.h"
#include "ConstantBuffer.h"
#include "LinearAllocator.h"

class ColorBuffer;
class DepthBuffer;
class Texture;
class GraphicsContext;
class ComputeContext;
class Color;
class GpuBuffer;
class StructuredBuffer;

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

//
// When deferred context and umap are used in a delay, the debugging 
// pixel shader is not possible. Because, debug button is inactive
// with the message "the pixel shader is not running".
// Therefore, we add an operation that maps or unmaps the constant 
// buffer immediately before the drawing operation. 
//
#define GRAPHICS_DEBUG

#ifdef GRAPHICS_DEBUG
struct ConstantBufferAllocator
{
	static LinearAllocatorPageManager sm_PageManager;
	static void DestroyAll()
	{
		sm_PageManager.Destroy();
	}
	void Create();
	void Destroy();
	std::vector<std::unique_ptr<LinearAllocationPage>> m_PagePool;
};
#endif

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
	CommandContext(ContextType Type);

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

	GraphicsContext& GetGraphicsContext() {
		return reinterpret_cast<GraphicsContext&>(*this);
	}

    ComputeContext& GetComputeContext() {
        return reinterpret_cast<ComputeContext&>(*this);
    }

    static void BeginQuery( ID3D11Query* pQueryDisjoint );
    static void EndQuery( ID3D11Query* pQueryDisjoint );
    void InsertTimeStamp( ID3D11Query* pQuery );
    static void ResolveTimeStamps( ID3D11Query* pQueryDisjoint, ID3D11Query** pQueryHeap, uint32_t NumQueries, D3D11_QUERY_DATA_TIMESTAMP_DISJOINT* pDisjoint, uint64_t* pBuffer );
	void PIXBeginEvent(const wchar_t* label);
	void PIXEndEvent(void);
	void PIXSetMarker(const wchar_t* label);

    void CopyBuffer( GpuResource& Dest, GpuResource& Src );
    void CopyBufferRegion( GpuResource& Dest, size_t DestOffset, GpuResource& Src, size_t SrcOffset, size_t NumBytes );
    void CopySubresource(GpuResource& Dest, UINT DestSubIndex, GpuResource& Src, UINT SrcSubIndex);
    void CopyCounter(GpuResource& Dest, size_t DestOffset, StructuredBuffer& Src);

	void SetConstants( UINT Slot, UINT NumConstants, const void* pConstants, BindList BindList );
	void SetConstants( UINT Slot, DWParam X, BindList BindList );
	void SetConstants( UINT Slot, DWParam X, DWParam Y, BindList BindList );
	void SetConstants( UINT Slot, DWParam X, DWParam Y, DWParam Z, BindList BindList );
	void SetConstants( UINT Slot, DWParam X, DWParam Y, DWParam Z, DWParam W, BindList BindList );
	void SetConstantBuffers( UINT Offset, UINT Count, const D3D11_BUFFER_HANDLE Handle[], BindList BindList );

	template <typename T> void SetDynamicConstantBufferView( UINT Slot, const ConstantBuffer<T>& Buffer, BindList BindList );
	void SetDynamicConstantBufferView( UINT Slot, size_t BufferSize, const void* BufferData, BindList Binds );
	void SetDynamicDescriptor( UINT Offset, const D3D11_SRV_HANDLE Handle, BindList Binds );
	void SetDynamicDescriptors( UINT Offset, UINT Count, const D3D11_SRV_HANDLE Handles[], BindList Binds );
    void SetDynamicSampler( UINT Offset, const D3D11_SAMPLER_HANDLE Handle, EPipelineBind Bind );
    void SetDynamicSamplers( UINT Offset, UINT Count, const D3D11_SAMPLER_HANDLE Handles[], BindList Binds );

	void UploadContstantBuffer( D3D11_BUFFER_HANDLE Handle, void const* Data, size_t Size );

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
	LinearAllocator m_GpuLinearAllocator;

#ifdef GRAPHICS_DEBUG
	ConstantBufferAllocator m_ConstantBufferAllocator;
#endif
};

class ComputeContext : public CommandContext
{
	friend ContextManager;

protected:
	ComputeContext();

public:
    static ComputeContext& Begin(const std::wstring& ID = L"");

	void SetPipelineState( ComputePSO& PSO );

	void SetConstants( UINT Slot, UINT NumConstants, const void* pConstants );
	void SetConstants( UINT Slot, DWParam X );
	void SetConstants( UINT Slot, DWParam X, DWParam Y );
	void SetConstants( UINT Slot, DWParam X, DWParam Y, DWParam Z );
	void SetConstants( UINT Slot, DWParam X, DWParam Y, DWParam Z, DWParam W );
	void SetConstantBuffers( UINT Offset, UINT Count, const D3D11_BUFFER_HANDLE Handle[] );

	void SetDynamicConstantBufferView( UINT Slot, size_t BufferSize, const void* BufferData );
    void SetDynamicDescriptor( UINT Offset, const D3D11_SRV_HANDLE Handle );
    void SetDynamicDescriptor( UINT Offset, const D3D11_UAV_HANDLE Handle );
	void SetDynamicDescriptors( UINT Offset, UINT Count, const D3D11_SRV_HANDLE Handles[] );
	void SetDynamicDescriptors( UINT Offset, UINT Count, const D3D11_UAV_HANDLE Handles[], const UINT *pUAVInitialCounts = nullptr );
    void SetDynamicSampler( UINT Offset, const D3D11_SAMPLER_HANDLE Handle );

    void Dispatch( size_t GroupCountX = 1, size_t GroupCountY = 1, size_t GroupCountZ = 1 );
    void Dispatch1D( size_t ThreadCountX, size_t GroupSizeX = 64);
    void Dispatch2D( size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX = 8, size_t GroupSizeY = 8);
    void Dispatch3D( size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ );
    void DispatchIndirect( GpuBuffer& ArgumentBuffer, size_t ArgumentBufferOffset = 0 );

	std::shared_ptr<ComputePipelineState> m_PSOState;
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

	void ClearColor( ColorBuffer& Target );
	void ClearDepth( DepthBuffer& Target );
	void ClearStencil( DepthBuffer& Target );
	void ClearDepthAndStencil( DepthBuffer& Target );

	void GenerateMips( D3D11_SRV_HANDLE SRV );

	void SetRenderTargets( UINT NumRTVs, const D3D11_RTV_HANDLE RTVs[] );
	void SetRenderTargets( UINT NumRTVs, const D3D11_RTV_HANDLE RTVs[], D3D11_DSV_HANDLE DSV );
	void SetRenderTarget( D3D11_RTV_HANDLE RTV ) { SetRenderTargets( 1, &RTV ); }
	void SetRenderTarget( D3D11_RTV_HANDLE RTV, D3D11_DSV_HANDLE DSV );
	void SetDepthStencilTarget( D3D11_DSV_HANDLE DSV ) { SetRenderTargets(0, nullptr, DSV); }
	void SetViewport( const D3D11_VIEWPORT& vp );
	void SetViewport( FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f );
	void SetScissor( const D3D11_RECT& rect );
	void SetScissor( UINT left, UINT top, UINT right, UINT bottom );
	void SetViewportAndScissor( const D3D11_VIEWPORT& vp, const D3D11_RECT& rect );
	void SetViewportAndScissor( UINT x, UINT y, UINT w, UINT h );
	void SetStencilRef( UINT StencilRef );
	void SetBlendFactor( const Color& BlendFactor );
	void SetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY Topology );

	void SetIndexBuffer( const D3D11_INDEX_BUFFER_VIEW& Buffer );
	void SetVertexBuffer( UINT Slot, const D3D11_VERTEX_BUFFER_VIEW& Buffer );
    void SetDynamicVB( UINT Slot, size_t NumVertices, size_t VertexStride, const void* VBData );

	void Draw( UINT VertexCount, UINT VertexStartOffset = 0 );
	void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0);
	void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
		UINT StartVertexLocation = 0, UINT StartInstanceLocation = 0);
	void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
		INT BaseVertexLocation, UINT StartInstanceLocation);
	void DrawIndirect( GpuBuffer& ArgumentBuffer, size_t ArgumentBufferOffset = 0 );
	void SetPipelineState( GraphicsPSO& PSO );

	std::shared_ptr<GraphicsPipelineState> m_PSOState;
};

inline void ComputeContext::Dispatch( size_t GroupCountX, size_t GroupCountY, size_t GroupCountZ )
{
    m_Context->Dispatch((UINT)GroupCountX, (UINT)GroupCountY, (UINT)GroupCountZ);
}

inline void ComputeContext::Dispatch1D( size_t ThreadCountX, size_t GroupSizeX )
{
    Dispatch( Math::DivideByMultiple(ThreadCountX, GroupSizeX), 1, 1 );
}

inline void ComputeContext::Dispatch2D( size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX, size_t GroupSizeY )
{
    Dispatch(
        Math::DivideByMultiple(ThreadCountX, GroupSizeX),
        Math::DivideByMultiple(ThreadCountY, GroupSizeY), 1);
}

inline void ComputeContext::Dispatch3D( size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ )
{
    Dispatch(
        Math::DivideByMultiple(ThreadCountX, GroupSizeX),
        Math::DivideByMultiple(ThreadCountY, GroupSizeY),
        Math::DivideByMultiple(ThreadCountZ, GroupSizeZ));
}

template<typename T>
inline void GraphicsContext::SetDynamicConstantBufferView( UINT Slot, const ConstantBuffer<T>& Buffer, BindList BindList )
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

inline void CommandContext::InsertTimeStamp( ID3D11Query* pQuery )
{
    m_Context->End( pQuery );
}

inline void CommandContext::PIXSetMarker( const wchar_t* label )
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

