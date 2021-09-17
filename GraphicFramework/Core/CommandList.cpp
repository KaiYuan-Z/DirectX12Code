#include "CommandList.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"

#ifndef RELEASE
	#include <d3d11_2.h>
	#include <pix3.h>
#endif


CCommandList::CCommandList(D3D12_COMMAND_LIST_TYPE Type) :
    m_Type(Type),
    m_DynamicViewDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
    m_DynamicSamplerDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
    m_CpuLinearAllocator(kCpuWritable), 
    m_GpuLinearAllocator(kGpuExclusive),
	m_OriginalCategory(kUnKown)
{
	m_IsInitialized = false;
	m_pDevice = nullptr;
	m_CommandList = nullptr;
    m_CurrentAllocator = nullptr;
    ZeroMemory(m_CurrentDescriptorHeaps, sizeof(m_CurrentDescriptorHeaps));
	ZeroMemory(m_CurrentDescriptorHeapStartOffsets, sizeof(m_CurrentDescriptorHeapStartOffsets));

    m_CurGraphicsRootSignature = nullptr;
    m_CurGraphicsPipelineState = nullptr;
    m_CurComputeRootSignature = nullptr;
    m_CurComputePipelineState = nullptr;
    m_NumBarriersToFlush = 0;
}

CCommandList::~CCommandList( void )
{
    if (m_CommandList != nullptr)
        m_CommandList->Release();
}

void CCommandList::Initialize(ID3D12Device* pDevice, CCommandQueueManager* pCommandQueueManager)
{
	if (!m_IsInitialized)
	{
		_ASSERTE(pDevice && pCommandQueueManager);
		m_pDevice = pDevice;
		m_pCommandQueueManager = pCommandQueueManager;
		m_pCommandQueueManager->GetCommandQueue(m_Type).__CreateNewD3D12CommandList(m_Type, &m_CommandList, &m_CurrentAllocator);
		m_IsInitialized = true;
	}
}

uint64_t CCommandList::Flush(bool WaitForCompletion)
{
	FlushResourceBarriers();

	ASSERT(m_CurrentAllocator != nullptr);

	uint64_t FenceValue = m_pCommandQueueManager->GetCommandQueue(m_Type).__ExecuteCommandList(m_CommandList);

	if (WaitForCompletion)
		m_pCommandQueueManager->GetCommandQueue(m_Type).WaitForFence(FenceValue);

	//
	// Reset the command list and restore previous state
	//

	m_CommandList->Reset(m_CurrentAllocator, nullptr);

	if (m_CurGraphicsRootSignature)
	{
		m_CommandList->SetGraphicsRootSignature(m_CurGraphicsRootSignature);
		m_CommandList->SetPipelineState(m_CurGraphicsPipelineState);
	}
	if (m_CurComputeRootSignature)
	{
		m_CommandList->SetComputeRootSignature(m_CurComputeRootSignature);
		m_CommandList->SetPipelineState(m_CurComputePipelineState);
	}

	_BindDescriptorHeaps();

	return FenceValue;
}

uint64_t CCommandList::__Finish(bool WaitForCompletion)
{
    ASSERT(m_Type == D3D12_COMMAND_LIST_TYPE_DIRECT || m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE);

    FlushResourceBarriers();

    ASSERT(m_CurrentAllocator != nullptr);

    CCommandQueue& Queue = m_pCommandQueueManager->GetCommandQueue(m_Type);

    uint64_t FenceValue = Queue.__ExecuteCommandList(m_CommandList);
    Queue.__DiscardAllocator(FenceValue, m_CurrentAllocator);
    m_CurrentAllocator = nullptr;

    m_CpuLinearAllocator.CleanupUsedPages(FenceValue);
    m_GpuLinearAllocator.CleanupUsedPages(FenceValue);
    m_DynamicViewDescriptorHeap.CleanupUsedHeaps(FenceValue);
    m_DynamicSamplerDescriptorHeap.CleanupUsedHeaps(FenceValue);

    if (WaitForCompletion)
        m_pCommandQueueManager->GetCommandQueue(m_Type).WaitForFence(FenceValue);

    return FenceValue;
}

void CCommandList::__Reset( void )
{
    // We only call __Reset() on previously freed command lists.  The command list persists, but we must
    // request a new allocator.
    ASSERT(m_CommandList != nullptr && m_CurrentAllocator == nullptr);
    m_CurrentAllocator = m_pCommandQueueManager->GetCommandQueue(m_Type).__RequestAllocator();
    m_CommandList->Reset(m_CurrentAllocator, nullptr);

    m_CurGraphicsRootSignature = nullptr;
    m_CurGraphicsPipelineState = nullptr;
    m_CurComputeRootSignature = nullptr;
    m_CurComputePipelineState = nullptr;
    m_NumBarriersToFlush = 0;

    _BindDescriptorHeaps();
}

void CCommandList::_BindDescriptorHeaps( void )
{
    UINT NonNullHeaps = 0;
    ID3D12DescriptorHeap* HeapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    for (UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        ID3D12DescriptorHeap* HeapIter = m_CurrentDescriptorHeaps[i];
        if (HeapIter != nullptr)
            HeapsToBind[NonNullHeaps++] = HeapIter;
    }

    if (NonNullHeaps > 0)
        m_CommandList->SetDescriptorHeaps(NonNullHeaps, HeapsToBind);
}

void CGraphicsCommandList::SetRenderTargets( UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV )
{
    m_CommandList->OMSetRenderTargets( NumRTVs, RTVs, FALSE, &DSV );
}

void CGraphicsCommandList::SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[])
{
    m_CommandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, nullptr);
}

void CGraphicsCommandList::BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
{
    m_CommandList->BeginQuery(QueryHeap, Type, HeapIndex);
}

void CGraphicsCommandList::EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
{
    m_CommandList->EndQuery(QueryHeap, Type, HeapIndex);
}

void CGraphicsCommandList::ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset)
{
    m_CommandList->ResolveQueryData(QueryHeap, Type, StartIndex, NumQueries, DestinationBuffer, DestinationBufferOffset);
}

void CGraphicsCommandList::ClearUAV( CGpuBuffer& Target )
{
    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
	const UINT ClearColor[4] = { 0, 0, 0, 0 };
    m_CommandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
}

void CComputeCommandList::ClearUAV( CGpuBuffer& Target )
{
    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
    const UINT ClearColor[4] = { 0, 0, 0, 0 };
    m_CommandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
}

void CGraphicsCommandList::ClearUAV( CColorBuffer& Target )
{
    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
    CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

    //TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
    const float* ClearColor = Target.GetClearColor().GetPtr();
    m_CommandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);
}

void CComputeCommandList::ClearUAV( CColorBuffer& Target )
{
    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
    CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

    //TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
    const float* ClearColor = Target.GetClearColor().GetPtr();
    m_CommandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);
}

void CGraphicsCommandList::ClearColor( CColorBuffer& Target )
{
    m_CommandList->ClearRenderTargetView(Target.GetRTV(), Target.GetClearColor().GetPtr(), 0, nullptr);
}

void CGraphicsCommandList::ClearDepth( CDepthBuffer& Target )
{
    m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr );
}

void CGraphicsCommandList::ClearStencil( CDepthBuffer& Target )
{
    m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
}

void CGraphicsCommandList::ClearDepthAndStencil( CDepthBuffer& Target )
{
    m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
}

void CGraphicsCommandList::SetViewportAndScissor( const D3D12_VIEWPORT& vp, const D3D12_RECT& rect )
{
    ASSERT(rect.left < rect.right && rect.top < rect.bottom);
    m_CommandList->RSSetViewports( 1, &vp );
    m_CommandList->RSSetScissorRects( 1, &rect );
}

void CGraphicsCommandList::SetViewport( const D3D12_VIEWPORT& vp )
{
    m_CommandList->RSSetViewports( 1, &vp );
}

void CGraphicsCommandList::SetViewport( FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth, FLOAT maxDepth )
{
    D3D12_VIEWPORT vp;
    vp.Width = w;
    vp.Height = h;
    vp.MinDepth = minDepth;
    vp.MaxDepth = maxDepth;
    vp.TopLeftX = x;
    vp.TopLeftY = y;
    m_CommandList->RSSetViewports( 1, &vp );
}

void CGraphicsCommandList::SetScissor( const D3D12_RECT& rect )
{
    ASSERT(rect.left < rect.right && rect.top < rect.bottom);
    m_CommandList->RSSetScissorRects( 1, &rect );
}

void CCommandList::TransitionResource(CGpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
{
    D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;

    if (m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        ASSERT((OldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == OldState);
        ASSERT((NewState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == NewState);
    }

    if (OldState != NewState)
    {
        ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = Resource.GetResource();
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = OldState;
        BarrierDesc.Transition.StateAfter = NewState;

        // Check to see if we already started the transition
        if (NewState == Resource.m_TransitioningState)
        {
            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
            Resource.m_TransitioningState = (D3D12_RESOURCE_STATES)-1;
        }
        else
            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

        Resource.m_UsageState = NewState;
    }
    else if (NewState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        InsertUAVBarrier(Resource, FlushImmediate);

    if (FlushImmediate || m_NumBarriersToFlush == 16)
        FlushResourceBarriers();
}

void CCommandList::BeginResourceTransition(CGpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
{
    // If it's already transitioning, finish that transition
    if (Resource.m_TransitioningState != (D3D12_RESOURCE_STATES)-1)
        TransitionResource(Resource, Resource.m_TransitioningState);

    D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;

    if (OldState != NewState)
    {
        ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = Resource.GetResource();
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = OldState;
        BarrierDesc.Transition.StateAfter = NewState;

        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

        Resource.m_TransitioningState = NewState;
    }

    if (FlushImmediate || m_NumBarriersToFlush == 16)
        FlushResourceBarriers();
}

void CCommandList::InsertUAVBarrier(CGpuResource& Resource, bool FlushImmediate)
{
    ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

    BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    BarrierDesc.UAV.pResource = Resource.GetResource();

    if (FlushImmediate)
        FlushResourceBarriers();
}

void CCommandList::InsertAliasBarrier(CGpuResource& Before, CGpuResource& After, bool FlushImmediate)
{
    ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

    BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    BarrierDesc.Aliasing.pResourceBefore = Before.GetResource();
    BarrierDesc.Aliasing.pResourceAfter = After.GetResource();

    if (FlushImmediate)
        FlushResourceBarriers();
}

void CCommandList::WriteBuffer( CGpuResource& Dest, size_t DestOffset, const void* BufferData, size_t NumBytes )
{
    ASSERT(BufferData != nullptr);
    SDynAlloc TempSpace = m_CpuLinearAllocator.Allocate( NumBytes, 512 );
	MemCopy(TempSpace.DataPtr, BufferData, NumBytes);
    CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes );
}

void CCommandList::FillBuffer( CGpuResource& Dest, size_t DestOffset, DWParam Value, size_t NumBytes )
{
    SDynAlloc TempSpace = m_CpuLinearAllocator.Allocate( NumBytes, 512 );
    __m128 VectorValue = _mm_set1_ps(Value.Float);
    SIMDMemFill(TempSpace.DataPtr, VectorValue, MathUtility::DivideByMultiple(NumBytes, 16));
    CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes );
}

void CCommandList::CopySubresource(CGpuResource& Dest, UINT DestSubIndex, CGpuResource& Src, UINT SrcSubIndex)
{
    FlushResourceBarriers();

    D3D12_TEXTURE_COPY_LOCATION DestLocation =
    {
        Dest.GetResource(),
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        DestSubIndex
    };

    D3D12_TEXTURE_COPY_LOCATION SrcLocation =
    {
        Src.GetResource(),
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        SrcSubIndex
    };

    m_CommandList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SrcLocation, nullptr);
}

void CCommandList::PIXBeginEvent(const wchar_t* label)
{
#ifdef RELEASE
	(label);
#else
	::PIXBeginEvent(m_CommandList, 0, label);
#endif
}

void CCommandList::PIXEndEvent(void)
{
#ifndef RELEASE
	::PIXEndEvent(m_CommandList);
#endif
}

void CCommandList::PIXSetMarker(const wchar_t* label)
{
#ifdef RELEASE
	(label);
#else
	::PIXSetMarker(m_CommandList, 0, label);
#endif
}
