#pragma once
#include "GraphicsCoreBase.h"
#include "CommandQueueManager.h"
#include "Color.h"
#include "PipelineState.h"
#include "RootSignature.h"
#include "GpuBuffer.h"
#include "StructuredBuffer.h"
#include "TextureLoader.h"
#include "PixelBuffer.h"
#include "DynamicDescriptorHeap.h"
#include "LinearAllocator.h"
#include "CommandSignature.h"


class CColorBuffer;
class CDepthBuffer;
class CTexture;
class CGraphicsCommandList;
class CComputeCommandList;

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

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )


struct NonCopyable
{
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable & operator=(const NonCopyable&) = delete;
};

enum ECommandListCategory
{
	kUnKown,
	kDirectCommandList,
	kGraphicsCommandList,
	kComputeCommandList
};


class CCommandList : NonCopyable
{
    friend class CCommandListManager;
	friend class CCommandManager;

public:
    ~CCommandList(void);

    // Prepare to render by reserving a command list and command allocator
    void Initialize(ID3D12Device* pDevice, CCommandQueueManager* pCommandQueueManager);

	// Flush existing commands to the GPU but keep the command list alive
	uint64_t Flush(bool WaitForCompletion = false);

	CCommandQueueManager* GetCommandQueueManager() { return m_pCommandQueueManager; }

    ID3D12GraphicsCommandList* GetD3D12CommandList() {
        return m_CommandList;
    }

	CGraphicsCommandList* QueryGraphicsCommandList() {
		ASSERT(m_OriginalCategory == kDirectCommandList, "The original category must be direct.");
		return reinterpret_cast<CGraphicsCommandList*>(this);
	}

	CComputeCommandList* QueryComputeCommandList() {
		ASSERT(m_OriginalCategory == kDirectCommandList, "The original category must be direct.");
		return reinterpret_cast<CComputeCommandList*>(this);
	}

    void CopyBuffer( CGpuResource& Dest, CGpuResource& Src );
    void CopyBufferRegion( CGpuResource& Dest, size_t DestOffset, CGpuResource& Src, size_t SrcOffset, size_t NumBytes );
    void CopySubresource(CGpuResource& Dest, UINT DestSubIndex, CGpuResource& Src, UINT SrcSubIndex);
    void CopyCounter(CGpuResource& Dest, size_t DestOffset, CStructuredBuffer& Src);
    void ResetCounter(CStructuredBuffer& Buf, uint32_t Value = 0);

    SDynAlloc ReserveUploadMemory(size_t SizeInBytes)
    {
        return m_CpuLinearAllocator.Allocate(SizeInBytes);
    }

    void WriteBuffer( CGpuResource& Dest, size_t DestOffset, const void* Data, size_t NumBytes );
    void FillBuffer( CGpuResource& Dest, size_t DestOffset, DWParam Value, size_t NumBytes );

    void TransitionResource(CGpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
    void BeginResourceTransition(CGpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
    void InsertUAVBarrier(CGpuResource& Resource, bool FlushImmediate = false);
    void InsertAliasBarrier(CGpuResource& Before, CGpuResource& After, bool FlushImmediate = false);
    inline void FlushResourceBarriers(void);

    void InsertTimeStamp( ID3D12QueryHeap* pQueryHeap, uint32_t QueryIdx );
    void ResolveTimeStamps( ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t NumQueries );
    void PIXBeginEvent(const wchar_t* label);
    void PIXEndEvent(void);
    void PIXSetMarker(const wchar_t* label);

    void SetDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr);
    void SetDescriptorHeaps( UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[]);

	void SetDynamicViewDescriptorTemporaryUserHeap(ID3D12DescriptorHeap* HeapPtr, UINT HeapSize, UINT StartOffset = 0);
	void SetDynamicSamplerDescriptorTemporaryUserHeap(ID3D12DescriptorHeap* HeapPtr, UINT HeapSize, UINT StartOffset = 0);

    void SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op);

	D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }

protected:
	void _SetID(const std::wstring& ID) { m_ID = ID; }
	void _SetCategory(ECommandListCategory Category) { m_OriginalCategory = Category; }
	ECommandListCategory _GetOriginalCategory() { return m_OriginalCategory; }
    void _BindDescriptorHeaps( void );

	bool m_IsInitialized;
	ID3D12Device* m_pDevice;
	CCommandQueueManager* m_pCommandQueueManager;
    ID3D12GraphicsCommandList* m_CommandList;
    ID3D12CommandAllocator* m_CurrentAllocator;

    ID3D12RootSignature* m_CurGraphicsRootSignature;
    ID3D12PipelineState* m_CurGraphicsPipelineState;
    ID3D12RootSignature* m_CurComputeRootSignature;
    ID3D12PipelineState* m_CurComputePipelineState;

    CDynamicDescriptorHeap m_DynamicViewDescriptorHeap;		// HEAP_TYPE_CBV_SRV_UAV
    CDynamicDescriptorHeap m_DynamicSamplerDescriptorHeap;	// HEAP_TYPE_SAMPLER

    D3D12_RESOURCE_BARRIER m_ResourceBarrierBuffer[16];
    UINT m_NumBarriersToFlush;

    ID3D12DescriptorHeap* m_CurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	UINT m_CurrentDescriptorHeapStartOffsets[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    CLinearAllocator m_CpuLinearAllocator;
    CLinearAllocator m_GpuLinearAllocator;

    std::wstring m_ID;
    D3D12_COMMAND_LIST_TYPE m_Type;
	ECommandListCategory m_OriginalCategory;

private:
	CCommandList(D3D12_COMMAND_LIST_TYPE Type);

	// Flush existing commands and release the current command list
	uint64_t __Finish(bool WaitForCompletion = false);

	void __Reset(void);
};

class CGraphicsCommandList : public CCommandList
{
public:
    void ClearUAV( CGpuBuffer& Target );
    void ClearUAV( CColorBuffer& Target );
    void ClearColor( CColorBuffer& Target );
    void ClearDepth( CDepthBuffer& Target );
    void ClearStencil( CDepthBuffer& Target );
    void ClearDepthAndStencil( CDepthBuffer& Target );

    void BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
    void EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
    void ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset);

    void SetRootSignature( const CRootSignature& RootSig );

    void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[]);
    void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV);
    void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV ) { SetRenderTargets(1, &RTV); }
    void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV ) { SetRenderTargets(1, &RTV, DSV); }
    void SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV ) { SetRenderTargets(0, nullptr, DSV); }

    void SetViewport( const D3D12_VIEWPORT& vp );
    void SetViewport( FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f );
    void SetScissor( const D3D12_RECT& rect );
    void SetScissor( UINT left, UINT top, UINT right, UINT bottom );
    void SetViewportAndScissor( const D3D12_VIEWPORT& vp, const D3D12_RECT& rect );
    void SetViewportAndScissor( UINT x, UINT y, UINT w, UINT h );
    void SetStencilRef( UINT StencilRef );
    void SetBlendFactor( CColor BlendFactor );
    void SetPrimitiveTopology( D3D12_PRIMITIVE_TOPOLOGY Topology );

    void SetPipelineState( const CGraphicsPSO& PSO );
    void SetConstantArray( UINT RootIndex, UINT NumConstants, const void* pConstants );
    void SetConstant( UINT RootIndex, DWParam Val, UINT Offset = 0 );
    void SetConstants( UINT RootIndex, DWParam X );
    void SetConstants( UINT RootIndex, DWParam X, DWParam Y );
    void SetConstants( UINT RootIndex, DWParam X, DWParam Y, DWParam Z );
    void SetConstants( UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W );
    void SetConstantBuffer( UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV );
    void SetDynamicConstantBufferView( UINT RootIndex, size_t BufferSize, const void* BufferData );
    void SetBufferSRV( UINT RootIndex, const CGpuBuffer& SRV, UINT64 Offset = 0);
    void SetBufferUAV( UINT RootIndex, const CGpuBuffer& UAV, UINT64 Offset = 0);
    void SetDescriptorTable( UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle );

    void SetDynamicDescriptor( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle );
    void SetDynamicDescriptors( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] );
    void SetDynamicSampler( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle );
    void SetDynamicSamplers( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] );

    void SetIndexBuffer( const D3D12_INDEX_BUFFER_VIEW& IBView );
    void SetVertexBuffer( UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView );
    void SetVertexBuffers( UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[] );
    void SetDynamicVB( UINT Slot, size_t NumVertices, size_t VertexStride, const void* VBData );
    void SetDynamicIB( size_t IndexCount, const uint16_t* IBData );
    void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData);

    void Draw( UINT VertexCount, UINT VertexStartOffset = 0 );
    void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0);
    void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
        UINT StartVertexLocation = 0, UINT StartInstanceLocation = 0);
    void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
        INT BaseVertexLocation, UINT StartInstanceLocation);
    void DrawIndirect(CCommandSignature& CommandSig, CGpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset = 0 );
    void ExecuteIndirect(CCommandSignature& CommandSig, CGpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
        uint32_t MaxCommands = 1, CGpuBuffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0);

	// CommitCachedDrawResourcesManually() is only for raytracing utility's bottom level code.
	void CommitCachedDrawResourcesManually();

private:
};

class CComputeCommandList : public CCommandList
{
public:
    void ClearUAV( CGpuBuffer& Target );
    void ClearUAV( CColorBuffer& Target );

    void SetRootSignature( const CRootSignature& RootSig );

    void SetPipelineState( const CComputePSO& PSO );
    void SetConstantArray( UINT RootIndex, UINT NumConstants, const void* pConstants );
    void SetConstant( UINT RootIndex, DWParam Val, UINT Offset = 0 );
    void SetConstants( UINT RootIndex, DWParam X );
    void SetConstants( UINT RootIndex, DWParam X, DWParam Y );
    void SetConstants( UINT RootIndex, DWParam X, DWParam Y, DWParam Z );
    void SetConstants( UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W );
    void SetConstantBuffer( UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV );
    void SetDynamicConstantBufferView( UINT RootIndex, size_t BufferSize, const void* BufferData );
    void SetDynamicSRV( UINT RootIndex, size_t BufferSize, const void* BufferData ); 
    void SetBufferSRV( UINT RootIndex, const CGpuBuffer& SRV, UINT64 Offset = 0);
    void SetBufferUAV( UINT RootIndex, const CGpuBuffer& UAV, UINT64 Offset = 0);
    void SetDescriptorTable( UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle );

    void SetDynamicDescriptor( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle );
    void SetDynamicDescriptors( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] );
    void SetDynamicSampler( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle );
    void SetDynamicSamplers( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] );

    void Dispatch( size_t GroupCountX = 1, size_t GroupCountY = 1, size_t GroupCountZ = 1 );
    void Dispatch1D( size_t ThreadCountX, size_t GroupSizeX = 64);
    void Dispatch2D( size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX = 8, size_t GroupSizeY = 8);
    void Dispatch3D( size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ );
    void DispatchIndirect(CCommandSignature& CommandSig, CGpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset = 0 );
    void ExecuteIndirect(CCommandSignature& CommandSig, CGpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
        uint32_t MaxCommands = 1, CGpuBuffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0);

	// CommitCachedDrawResourcesManually() is only for raytracing utility's bottom level code.
	void CommitCachedDispatchResourcesManually();

private:
};

inline void CCommandList::FlushResourceBarriers( void )
{
    if (m_NumBarriersToFlush > 0)
    {
        m_CommandList->ResourceBarrier(m_NumBarriersToFlush, m_ResourceBarrierBuffer);
        m_NumBarriersToFlush = 0;
    }
}

inline void CGraphicsCommandList::SetRootSignature( const CRootSignature& RootSig )
{
    if (RootSig.GetSignature() == m_CurGraphicsRootSignature)
        return;

    m_CommandList->SetGraphicsRootSignature(m_CurGraphicsRootSignature = RootSig.GetSignature());

    m_DynamicViewDescriptorHeap.ParseGraphicsRootSignature(RootSig);
    m_DynamicSamplerDescriptorHeap.ParseGraphicsRootSignature(RootSig);
}

inline void CComputeCommandList::SetRootSignature( const CRootSignature& RootSig )
{
    if (RootSig.GetSignature() == m_CurComputeRootSignature)
        return;

    m_CommandList->SetComputeRootSignature(m_CurComputeRootSignature = RootSig.GetSignature());

    m_DynamicViewDescriptorHeap.ParseComputeRootSignature(RootSig);
    m_DynamicSamplerDescriptorHeap.ParseComputeRootSignature(RootSig);
}

inline void CGraphicsCommandList::SetPipelineState( const CGraphicsPSO& PSO )
{
    ID3D12PipelineState* PipelineState = PSO.GetPipelineStateObject();
    if (PipelineState == m_CurGraphicsPipelineState)
        return;

    m_CommandList->SetPipelineState(PipelineState);
    m_CurGraphicsPipelineState = PipelineState;
}

inline void CComputeCommandList::SetPipelineState( const CComputePSO& PSO )
{
    ID3D12PipelineState* PipelineState = PSO.GetPipelineStateObject();
    if (PipelineState == m_CurComputePipelineState)
        return;

    m_CommandList->SetPipelineState(PipelineState);
    m_CurComputePipelineState = PipelineState;
}

inline void CGraphicsCommandList::SetViewportAndScissor( UINT x, UINT y, UINT w, UINT h )
{
    SetViewport((float)x, (float)y, (float)w, (float)h);
    SetScissor(x, y, x + w, y + h);
}

inline void CGraphicsCommandList::SetScissor( UINT left, UINT top, UINT right, UINT bottom )
{
    SetScissor(CD3DX12_RECT(left, top, right, bottom));
}

inline void CGraphicsCommandList::SetStencilRef( UINT ref )
{
    m_CommandList->OMSetStencilRef( ref );
}

inline void CGraphicsCommandList::SetBlendFactor( CColor BlendFactor )
{
    m_CommandList->OMSetBlendFactor( BlendFactor.GetPtr() );
}

inline void CGraphicsCommandList::SetPrimitiveTopology( D3D12_PRIMITIVE_TOPOLOGY Topology )
{
    m_CommandList->IASetPrimitiveTopology(Topology);
}

inline void CComputeCommandList::SetConstantArray( UINT RootEntry, UINT NumConstants, const void* pConstants )
{
    m_CommandList->SetComputeRoot32BitConstants( RootEntry, NumConstants, pConstants, 0 );
}

inline void CComputeCommandList::SetConstant( UINT RootEntry, DWParam Val, UINT Offset )
{
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, Val.Uint, Offset );
}

inline void CComputeCommandList::SetConstants( UINT RootEntry, DWParam X )
{
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, X.Uint, 0 );
}

inline void CComputeCommandList::SetConstants( UINT RootEntry, DWParam X, DWParam Y )
{
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, X.Uint, 0 );
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, Y.Uint, 1 );
}

inline void CComputeCommandList::SetConstants( UINT RootEntry, DWParam X, DWParam Y, DWParam Z )
{
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, X.Uint, 0 );
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, Y.Uint, 1 );
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, Z.Uint, 2 );
}

inline void CComputeCommandList::SetConstants( UINT RootEntry, DWParam X, DWParam Y, DWParam Z, DWParam W )
{
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, X.Uint, 0 );
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, Y.Uint, 1 );
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, Z.Uint, 2 );
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, W.Uint, 3 );
}

inline void CGraphicsCommandList::SetConstantArray( UINT RootIndex, UINT NumConstants, const void* pConstants )
{
    m_CommandList->SetGraphicsRoot32BitConstants( RootIndex, NumConstants, pConstants, 0 );
}

inline void CGraphicsCommandList::SetConstant( UINT RootEntry, DWParam Val, UINT Offset )
{
    m_CommandList->SetGraphicsRoot32BitConstant( RootEntry, Val.Uint, Offset );
}

inline void CGraphicsCommandList::SetConstants( UINT RootIndex, DWParam X )
{
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, X.Uint, 0 );
}

inline void CGraphicsCommandList::SetConstants( UINT RootIndex, DWParam X, DWParam Y )
{
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, X.Uint, 0 );
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, Y.Uint, 1 );
}

inline void CGraphicsCommandList::SetConstants( UINT RootIndex, DWParam X, DWParam Y, DWParam Z )
{
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, X.Uint, 0 );
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, Y.Uint, 1 );
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, Z.Uint, 2 );
}

inline void CGraphicsCommandList::SetConstants( UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W )
{
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, X.Uint, 0 );
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, Y.Uint, 1 );
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, Z.Uint, 2 );
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, W.Uint, 3 );
}

inline void CComputeCommandList::SetConstantBuffer( UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV )
{
    m_CommandList->SetComputeRootConstantBufferView(RootIndex, CBV);
}

inline void CGraphicsCommandList::SetConstantBuffer( UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV )
{
    m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, CBV);
}

inline void CGraphicsCommandList::SetDynamicConstantBufferView( UINT RootIndex, size_t BufferSize, const void* BufferData )
{
    ASSERT(BufferData != nullptr);
    SDynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
    MemCopy(cb.DataPtr, BufferData, BufferSize);
    m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, cb.GpuAddress);
}

inline void CComputeCommandList::SetDynamicConstantBufferView( UINT RootIndex, size_t BufferSize, const void* BufferData )
{
    ASSERT(BufferData != nullptr);
    SDynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
	MemCopy(cb.DataPtr, BufferData, BufferSize);
    m_CommandList->SetComputeRootConstantBufferView(RootIndex, cb.GpuAddress);
}

inline void CGraphicsCommandList::SetDynamicVB( UINT Slot, size_t NumVertices, size_t VertexStride, const void* VertexData )
{
    ASSERT(VertexData != nullptr);

    size_t BufferSize = NumVertices * VertexStride;
    SDynAlloc vb = m_CpuLinearAllocator.Allocate(BufferSize);

	MemCopy(vb.DataPtr, VertexData, BufferSize);

    D3D12_VERTEX_BUFFER_VIEW VBView;
    VBView.BufferLocation = vb.GpuAddress;
    VBView.SizeInBytes = (UINT)BufferSize;
    VBView.StrideInBytes = (UINT)VertexStride;

    m_CommandList->IASetVertexBuffers(Slot, 1, &VBView);
}

inline void CGraphicsCommandList::SetDynamicIB( size_t IndexCount, const uint16_t* IndexData )
{
    ASSERT(IndexData != nullptr);

    size_t BufferSize = IndexCount * sizeof(uint16_t);
    SDynAlloc ib = m_CpuLinearAllocator.Allocate(BufferSize);

	MemCopy(ib.DataPtr, IndexData, BufferSize);

    D3D12_INDEX_BUFFER_VIEW IBView;
    IBView.BufferLocation = ib.GpuAddress;
    IBView.SizeInBytes = (UINT)(IndexCount * sizeof(uint16_t));
    IBView.Format = DXGI_FORMAT_R16_UINT;

    m_CommandList->IASetIndexBuffer(&IBView);
}

inline void CGraphicsCommandList::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
{
    ASSERT(BufferData != nullptr);
    SDynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
	MemCopy(cb.DataPtr, BufferData, BufferSize);
    m_CommandList->SetGraphicsRootShaderResourceView(RootIndex, cb.GpuAddress);
}

inline void CComputeCommandList::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
{
    ASSERT(BufferData != nullptr);
    SDynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
	MemCopy(cb.DataPtr, BufferData, BufferSize);
    m_CommandList->SetComputeRootShaderResourceView(RootIndex, cb.GpuAddress);
}

inline void CGraphicsCommandList::SetBufferSRV( UINT RootIndex, const CGpuBuffer& SRV, UINT64 Offset)
{
    ASSERT((SRV.m_UsageState & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
    m_CommandList->SetGraphicsRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
}

inline void CComputeCommandList::SetBufferSRV( UINT RootIndex, const CGpuBuffer& SRV, UINT64 Offset)
{
	//ASSERT((SRV.m_UsageState & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0);
    m_CommandList->SetComputeRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
}

inline void CGraphicsCommandList::SetBufferUAV( UINT RootIndex, const CGpuBuffer& UAV, UINT64 Offset)
{
    ASSERT((UAV.m_UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
    m_CommandList->SetGraphicsRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
}

inline void CComputeCommandList::SetBufferUAV( UINT RootIndex, const CGpuBuffer& UAV, UINT64 Offset)
{
    ASSERT((UAV.m_UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
    m_CommandList->SetComputeRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
}

inline void CComputeCommandList::Dispatch( size_t GroupCountX, size_t GroupCountY, size_t GroupCountZ )
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
    m_CommandList->Dispatch((UINT)GroupCountX, (UINT)GroupCountY, (UINT)GroupCountZ);
}

inline void CComputeCommandList::Dispatch1D( size_t ThreadCountX, size_t GroupSizeX )
{
    Dispatch( MathUtility::DivideByMultiple(ThreadCountX, GroupSizeX), 1, 1 );
}

inline void CComputeCommandList::Dispatch2D( size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX, size_t GroupSizeY )
{
    Dispatch(
        MathUtility::DivideByMultiple(ThreadCountX, GroupSizeX),
        MathUtility::DivideByMultiple(ThreadCountY, GroupSizeY), 1);
}

inline void CComputeCommandList::Dispatch3D( size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ )
{
    Dispatch(
        MathUtility::DivideByMultiple(ThreadCountX, GroupSizeX),
        MathUtility::DivideByMultiple(ThreadCountY, GroupSizeY),
        MathUtility::DivideByMultiple(ThreadCountZ, GroupSizeZ));
}

inline void CCommandList::SetDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr )
{
    if (m_CurrentDescriptorHeaps[Type] != HeapPtr)
    {
        m_CurrentDescriptorHeaps[Type] = HeapPtr;
        _BindDescriptorHeaps();
    }
}

inline void CCommandList::SetDescriptorHeaps( UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[] )
{
    bool AnyChanged = false;

    for (UINT i = 0; i < HeapCount; ++i)
    {
        if (m_CurrentDescriptorHeaps[Type[i]] != HeapPtrs[i])
        {
            m_CurrentDescriptorHeaps[Type[i]] = HeapPtrs[i];
            AnyChanged = true;
        }
    }

    if (AnyChanged)
        _BindDescriptorHeaps();
}

inline void CCommandList::SetDynamicViewDescriptorTemporaryUserHeap(ID3D12DescriptorHeap* HeapPtr, UINT HeapSize, UINT StartOffset)
{
	m_DynamicViewDescriptorHeap.SetTemporaryUserHeap(HeapPtr, HeapSize, StartOffset);
}

inline void CCommandList::SetDynamicSamplerDescriptorTemporaryUserHeap(ID3D12DescriptorHeap* HeapPtr, UINT HeapSize, UINT StartOffset)
{
	m_DynamicSamplerDescriptorHeap.SetTemporaryUserHeap(HeapPtr, HeapSize, StartOffset);
}

inline void CCommandList::SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op)
{
    m_CommandList->SetPredication(Buffer, BufferOffset, Op);
}

inline void CGraphicsCommandList::SetDynamicDescriptor( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle )
{
    SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
}

inline void CComputeCommandList::SetDynamicDescriptor( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle )
{
    SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
}

inline void CGraphicsCommandList::SetDynamicDescriptors( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] )
{
    m_DynamicViewDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
}

inline void CComputeCommandList::SetDynamicDescriptors( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] )
{
    m_DynamicViewDescriptorHeap.SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
}

inline void CGraphicsCommandList::SetDynamicSampler( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle )
{
    SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
}

inline void CGraphicsCommandList::SetDynamicSamplers( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] )
{
    m_DynamicSamplerDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
}

inline void CComputeCommandList::SetDynamicSampler( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle )
{
    SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
}

inline void CComputeCommandList::SetDynamicSamplers( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] )
{
    m_DynamicSamplerDescriptorHeap.SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
}

inline void CGraphicsCommandList::SetDescriptorTable( UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle )
{
    m_CommandList->SetGraphicsRootDescriptorTable( RootIndex, FirstHandle );
}

inline void CComputeCommandList::SetDescriptorTable( UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle )
{
    m_CommandList->SetComputeRootDescriptorTable( RootIndex, FirstHandle );
}

inline void CGraphicsCommandList::SetIndexBuffer( const D3D12_INDEX_BUFFER_VIEW& IBView )
{
    m_CommandList->IASetIndexBuffer(&IBView);
}

inline void CGraphicsCommandList::SetVertexBuffer( UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView )
{
    SetVertexBuffers(Slot, 1, &VBView);
}

inline void CGraphicsCommandList::SetVertexBuffers( UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[] )
{
    m_CommandList->IASetVertexBuffers(StartSlot, Count, VBViews);
}

inline void CGraphicsCommandList::Draw(UINT VertexCount, UINT VertexStartOffset)
{
    DrawInstanced(VertexCount, 1, VertexStartOffset, 0);
}

inline void CGraphicsCommandList::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

inline void CGraphicsCommandList::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
    UINT StartVertexLocation, UINT StartInstanceLocation)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

inline void CGraphicsCommandList::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
    INT BaseVertexLocation, UINT StartInstanceLocation)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

inline void CGraphicsCommandList::ExecuteIndirect(CCommandSignature& CommandSig,
    CGpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset,
    uint32_t MaxCommands, CGpuBuffer* CommandCounterBuffer, uint64_t CounterOffset)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_CommandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands,
        ArgumentBuffer.GetResource(), ArgumentStartOffset,
        CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset);
}

inline void CGraphicsCommandList::CommitCachedDrawResourcesManually()
{
	FlushResourceBarriers();
	m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
}

inline void CGraphicsCommandList::DrawIndirect(CCommandSignature& CommandSig, CGpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset)
{
    ExecuteIndirect(CommandSig, ArgumentBuffer, ArgumentBufferOffset);
}

inline void CComputeCommandList::ExecuteIndirect(CCommandSignature& CommandSig,
    CGpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset,
    uint32_t MaxCommands, CGpuBuffer* CommandCounterBuffer, uint64_t CounterOffset)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
    m_CommandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands,
        ArgumentBuffer.GetResource(), ArgumentStartOffset,
        CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset);
}

inline void CComputeCommandList::CommitCachedDispatchResourcesManually()
{
	FlushResourceBarriers();
	m_DynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
	m_DynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
}

inline void CComputeCommandList::DispatchIndirect(CCommandSignature& CommandSig, CGpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset )
{
    ExecuteIndirect(CommandSig, ArgumentBuffer, ArgumentBufferOffset);
}

inline void CCommandList::CopyBuffer( CGpuResource& Dest, CGpuResource& Src )
{
    TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    FlushResourceBarriers();
    m_CommandList->CopyResource(Dest.GetResource(), Src.GetResource());
}

inline void CCommandList::CopyBufferRegion( CGpuResource& Dest, size_t DestOffset, CGpuResource& Src, size_t SrcOffset, size_t NumBytes )
{
    TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    //TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    FlushResourceBarriers();
    m_CommandList->CopyBufferRegion( Dest.GetResource(), DestOffset, Src.GetResource(), SrcOffset, NumBytes);
}

inline void CCommandList::CopyCounter(CGpuResource& Dest, size_t DestOffset, CStructuredBuffer& Src)
{
    TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionResource(Src.GetCounterBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    FlushResourceBarriers();
    m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, Src.GetCounterBuffer().GetResource(), 0, 4);
}

inline void CCommandList::ResetCounter(CStructuredBuffer& Buf, uint32_t Value )
{
    FillBuffer(Buf.GetCounterBuffer(), 0, Value, sizeof(uint32_t));
    TransitionResource(Buf.GetCounterBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

inline void CCommandList::InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t QueryIdx)
{
    m_CommandList->EndQuery(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, QueryIdx);
}

inline void CCommandList::ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t NumQueries)
{
    m_CommandList->ResolveQueryData(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, NumQueries, pReadbackHeap, 0);
}
