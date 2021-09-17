#include "CommandQueueManager.h"
#include "CommandListManager.h"
#include "DynamicDescriptorHeap.h"

//
// DynamicDescriptorHeap Implementation
//

bool CDynamicDescriptorHeap::sm_IsInitialized = false;
ID3D12Device* CDynamicDescriptorHeap::sm_pDevice = nullptr;
std::mutex CDynamicDescriptorHeap::sm_Mutex;
std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> CDynamicDescriptorHeap::sm_DescriptorHeapPool[2];
std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>> CDynamicDescriptorHeap::sm_RetiredDescriptorHeaps[2];
std::queue<ID3D12DescriptorHeap*> CDynamicDescriptorHeap::sm_AvailableDescriptorHeaps[2];

void CDynamicDescriptorHeap::sInitialize(ID3D12Device* pDevice)
{
	if (!sm_IsInitialized)
	{
		_ASSERTE(pDevice);
		sm_pDevice = pDevice;
		sm_IsInitialized = true;
	}
}

void CDynamicDescriptorHeap::sShutdown()
{
	if (sm_IsInitialized)
	{
		sm_DescriptorHeapPool[0].clear();
		sm_DescriptorHeapPool[1].clear();
		sm_IsInitialized = false;
	}
}

ID3D12DescriptorHeap* CDynamicDescriptorHeap::__sRequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, CCommandQueueManager* pCommandQueueManager)
{
    std::lock_guard<std::mutex> LockGuard(sm_Mutex);

	_ASSERTE(pCommandQueueManager);

    uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

    while (!sm_RetiredDescriptorHeaps[idx].empty() && pCommandQueueManager->GetCommandQueue(D3D12_COMMAND_LIST_TYPE(sm_RetiredDescriptorHeaps[idx].front().first >> 56)).IsFenceComplete(sm_RetiredDescriptorHeaps[idx].front().first))
    {
        sm_AvailableDescriptorHeaps[idx].push(sm_RetiredDescriptorHeaps[idx].front().second);
        sm_RetiredDescriptorHeaps[idx].pop();
    }

    if (!sm_AvailableDescriptorHeaps[idx].empty())
    {
        ID3D12DescriptorHeap* HeapPtr = sm_AvailableDescriptorHeaps[idx].front();
        sm_AvailableDescriptorHeaps[idx].pop();
        return HeapPtr;
    }
    else
    {
        D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
        HeapDesc.Type = HeapType;
        HeapDesc.NumDescriptors = kNumDescriptorsPerHeap;
        HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        HeapDesc.NodeMask = 1;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> HeapPtr;
        ASSERT_SUCCEEDED(sm_pDevice->CreateDescriptorHeap(&HeapDesc, MY_IID_PPV_ARGS(&HeapPtr)));
        sm_DescriptorHeapPool[idx].emplace_back(HeapPtr);
        return HeapPtr.Get();
    }
}

void CDynamicDescriptorHeap::__sDiscardDescriptorHeaps( D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValue, const std::vector<ID3D12DescriptorHeap*>& UsedHeaps )
{
    uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;
    std::lock_guard<std::mutex> LockGuard(sm_Mutex);
    for (auto iter = UsedHeaps.begin(); iter != UsedHeaps.end(); ++iter)
        sm_RetiredDescriptorHeaps[idx].push(std::make_pair(FenceValue, *iter));
}

void CDynamicDescriptorHeap::__RetireCurrentHeap( void )
{
    // Don't retire unused heaps.
    if (m_CurrentOffset == 0)
    {
        ASSERT(m_CurrentHeapPtr == nullptr);
        return;
    }

    ASSERT(m_CurrentHeapPtr != nullptr);
    m_RetiredHeaps.push_back(m_CurrentHeapPtr);
    m_CurrentHeapPtr = nullptr;
    m_CurrentOffset = 0;
}

void CDynamicDescriptorHeap::__RetireUsedHeaps( uint64_t fenceValue )
{
    __sDiscardDescriptorHeaps(m_DescriptorType, fenceValue, m_RetiredHeaps);
    m_RetiredHeaps.clear();
}

CDynamicDescriptorHeap::CDynamicDescriptorHeap(CCommandList& CommandList, D3D12_DESCRIPTOR_HEAP_TYPE HeapType)
    : m_OwningCommandList(CommandList), m_DescriptorType(HeapType)
{
    m_CurrentHeapPtr = nullptr;
	m_UserHeapPtr = nullptr;
    m_CurrentOffset = 0;
	m_UserHeapSize = 0;
	m_UserHeapStartOffset = 0;
    m_DescriptorSize = sm_pDevice->GetDescriptorHandleIncrementSize(HeapType);
}

CDynamicDescriptorHeap::~CDynamicDescriptorHeap()
{
}

void CDynamicDescriptorHeap::SetTemporaryUserHeap(ID3D12DescriptorHeap* HeapPtr, UINT HeapSize, UINT StartOffset)
{
	_ASSERTE(HeapPtr && HeapSize > 0 && HeapSize > StartOffset);
	m_UserHeapPtr = HeapPtr;
	m_UserHeapSize = HeapSize;
	m_UserHeapStartOffset = StartOffset;
	m_UserHeapFirstDescriptor = CDescriptorHandle(HeapPtr->GetCPUDescriptorHandleForHeapStart(), HeapPtr->GetGPUDescriptorHandleForHeapStart());
	m_OwningCommandList.SetDescriptorHeap(m_DescriptorType, m_UserHeapPtr);
}

void CDynamicDescriptorHeap::CleanupUsedHeaps( uint64_t fenceValue )
{
    __RetireCurrentHeap();
    __RetireUsedHeaps(fenceValue);
    m_GraphicsHandleCache.ClearCache();
    m_ComputeHandleCache.ClearCache();
}

inline ID3D12DescriptorHeap* CDynamicDescriptorHeap::__GetHeapPointer()
{
    if (m_CurrentHeapPtr == nullptr)
    {
        ASSERT(m_CurrentOffset == 0);
        m_CurrentHeapPtr = __sRequestDescriptorHeap(m_DescriptorType, m_OwningCommandList.GetCommandQueueManager());
        m_FirstDescriptor = CDescriptorHandle(
            m_CurrentHeapPtr->GetCPUDescriptorHandleForHeapStart(),
            m_CurrentHeapPtr->GetGPUDescriptorHandleForHeapStart());
    }

    return m_CurrentHeapPtr;
}

uint32_t CDynamicDescriptorHeap::SDescriptorHandleCache::ComputeStagedSize()
{
    // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
    uint32_t NeededSpace = 0;
    uint32_t RootIndex;
    uint32_t StaleParams = m_StaleRootParamsBitMap;
    while (_BitScanForward((unsigned long*)&RootIndex, StaleParams))
    {
        StaleParams ^= (1 << RootIndex);

        uint32_t MaxSetHandle;
        ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap),
            "Root entry marked as stale but has no stale descriptors");

        NeededSpace += MaxSetHandle + 1;
    }
    return NeededSpace;
}

void CDynamicDescriptorHeap::SDescriptorHandleCache::CopyAndBindStaleTables(
    D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t DescriptorSize,
    CDescriptorHandle DestHandleStart, ID3D12GraphicsCommandList* CmdList,
    void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
    uint32_t StaleParamCount = 0;
    uint32_t TableSize[SDescriptorHandleCache::kMaxNumDescriptorTables];
    uint32_t RootIndices[SDescriptorHandleCache::kMaxNumDescriptorTables];
    //uint32_t NeededSpace = 0;
    uint32_t RootIndex;

    // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
    uint32_t StaleParams = m_StaleRootParamsBitMap;
    while (_BitScanForward((unsigned long*)&RootIndex, StaleParams))
    {
        RootIndices[StaleParamCount] = RootIndex;
        StaleParams ^= (1 << RootIndex);

        uint32_t MaxSetHandle;
        ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap),
            "Root entry marked as stale but has no stale descriptors");

        //NeededSpace += MaxSetHandle + 1;
        TableSize[StaleParamCount] = MaxSetHandle + 1;

        ++StaleParamCount;
    }

    ASSERT(StaleParamCount <= SDescriptorHandleCache::kMaxNumDescriptorTables,
        "We're only equipped to handle so many descriptor tables");

    m_StaleRootParamsBitMap = 0;

    static const uint32_t kMaxDescriptorsPerCopy = 16;
    UINT NumDestDescriptorRanges = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[kMaxDescriptorsPerCopy];
    UINT pDestDescriptorRangeSizes[kMaxDescriptorsPerCopy];

    UINT NumSrcDescriptorRanges = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangeStarts[kMaxDescriptorsPerCopy];
    UINT pSrcDescriptorRangeSizes[kMaxDescriptorsPerCopy];

    for (uint32_t i = 0; i < StaleParamCount; ++i)
    {
        RootIndex = RootIndices[i];
        (CmdList->*SetFunc)(RootIndex, DestHandleStart.GetGpuHandle());

        SDescriptorTableCache& RootDescTable = m_RootDescriptorTable[RootIndex];

        D3D12_CPU_DESCRIPTOR_HANDLE* SrcHandles = RootDescTable.TableStart;
        uint64_t SetHandles = (uint64_t)RootDescTable.AssignedHandlesBitMap;
        D3D12_CPU_DESCRIPTOR_HANDLE CurDest = DestHandleStart.GetCpuHandle();
        DestHandleStart += TableSize[i] * DescriptorSize;

        unsigned long SkipCount;
        while (_BitScanForward64(&SkipCount, SetHandles))
        {
            // Skip over unset descriptor handles
            SetHandles >>= SkipCount;
            SrcHandles += SkipCount;
            CurDest.ptr += SkipCount * DescriptorSize;

            unsigned long DescriptorCount;
            _BitScanForward64(&DescriptorCount, ~SetHandles);
            SetHandles >>= DescriptorCount;

            // If we run out of temp room, copy what we've got so far
            if (NumSrcDescriptorRanges + DescriptorCount > kMaxDescriptorsPerCopy)
            {
				sm_pDevice->CopyDescriptors(
					NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
					NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
					Type);

                NumSrcDescriptorRanges = 0;
                NumDestDescriptorRanges = 0;
            }

            // Setup destination range
            pDestDescriptorRangeStarts[NumDestDescriptorRanges] = CurDest;
            pDestDescriptorRangeSizes[NumDestDescriptorRanges] = DescriptorCount;
            ++NumDestDescriptorRanges;

            // Setup source ranges (one descriptor each because we don't assume they are contiguous)
            for (uint32_t j = 0; j < DescriptorCount; ++j)
            {
                pSrcDescriptorRangeStarts[NumSrcDescriptorRanges] = SrcHandles[j];
                pSrcDescriptorRangeSizes[NumSrcDescriptorRanges] = 1;
                ++NumSrcDescriptorRanges;
            }

            // Move the destination pointer forward by the number of descriptors we will copy
            SrcHandles += DescriptorCount;
            CurDest.ptr += DescriptorCount * DescriptorSize;
        }
    }

	sm_pDevice->CopyDescriptors(
		NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
		NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
		Type);
}
    
void CDynamicDescriptorHeap::__CopyAndBindStagedTables( SDescriptorHandleCache& HandleCache, ID3D12GraphicsCommandList* CmdList,
    void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
	uint32_t NeededSize = HandleCache.ComputeStagedSize();
	if (m_UserHeapPtr == nullptr)
	{	
		if (!__HasSpace(NeededSize))
		{
			__RetireCurrentHeap();
		}

		// This can trigger the creation of a new heap
		m_OwningCommandList.SetDescriptorHeap(m_DescriptorType, __GetHeapPointer());

		HandleCache.CopyAndBindStaleTables(m_DescriptorType, m_DescriptorSize, __Allocate(NeededSize), CmdList, SetFunc);
	}
	else
	{
		ASSERT(NeededSize <= m_UserHeapSize - m_UserHeapStartOffset);

		CDescriptorHandle DestHandleStart = m_UserHeapFirstDescriptor + m_UserHeapStartOffset * m_DescriptorSize;
		HandleCache.CopyAndBindStaleTables(m_DescriptorType, m_DescriptorSize, DestHandleStart, CmdList, SetFunc);

		__RetireUserHeap();
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE CDynamicDescriptorHeap::UploadDirect( D3D12_CPU_DESCRIPTOR_HANDLE Handle )
{
	if (m_UserHeapPtr == nullptr)
	{
		if (!__HasSpace(1))
		{
			__RetireCurrentHeap();
		}

		m_OwningCommandList.SetDescriptorHeap(m_DescriptorType, __GetHeapPointer());

		CDescriptorHandle DestHandle = m_FirstDescriptor + m_CurrentOffset * m_DescriptorSize;
		m_CurrentOffset += 1;
		sm_pDevice->CopyDescriptorsSimple(1, DestHandle.GetCpuHandle(), Handle, m_DescriptorType);

		return DestHandle.GetGpuHandle();
	}
	else
	{
		ASSERT(m_UserHeapSize - m_UserHeapStartOffset >= 1);

		CDescriptorHandle DestHandle = m_UserHeapFirstDescriptor + m_UserHeapStartOffset * m_DescriptorSize;
		sm_pDevice->CopyDescriptorsSimple(1, DestHandle.GetCpuHandle(), Handle, m_DescriptorType);

		__RetireUserHeap();

		return DestHandle.GetGpuHandle();
	}
}

void CDynamicDescriptorHeap::SDescriptorHandleCache::StageDescriptorHandles( UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] )
{
    ASSERT(((1 << RootIndex) & m_RootDescriptorTablesBitMap) != 0, "Root parameter is not a CBV_SRV_UAV descriptor table");
    ASSERT(Offset + NumHandles <= m_RootDescriptorTable[RootIndex].TableSize);

    SDescriptorTableCache& TableCache = m_RootDescriptorTable[RootIndex];
    D3D12_CPU_DESCRIPTOR_HANDLE* CopyDest = TableCache.TableStart + Offset;
    for (UINT i = 0; i < NumHandles; ++i)
        CopyDest[i] = Handles[i];
    TableCache.AssignedHandlesBitMap |= ((1 << NumHandles) - 1) << Offset;
    m_StaleRootParamsBitMap |= (1 << RootIndex);
}

void CDynamicDescriptorHeap::SDescriptorHandleCache::ParseRootSignature( D3D12_DESCRIPTOR_HEAP_TYPE Type, const CRootSignature& RootSig )
{
    UINT CurrentOffset = 0;

    ASSERT(RootSig.m_NumParameters <= 16, "Maybe we need to support something greater");

    m_StaleRootParamsBitMap = 0;
    m_RootDescriptorTablesBitMap = (Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? RootSig.m_SamplerTableBitMap : RootSig.m_DescriptorTableBitMap);

    unsigned long TableParams = m_RootDescriptorTablesBitMap;
    unsigned long RootIndex;
    while (_BitScanForward(&RootIndex, TableParams))
    {
        TableParams ^= (1 << RootIndex);

        UINT TableSize = RootSig.m_DescriptorTableSize[RootIndex];
        ASSERT(TableSize > 0);

        SDescriptorTableCache& RootDescriptorTable = m_RootDescriptorTable[RootIndex];
        RootDescriptorTable.AssignedHandlesBitMap = 0;
        RootDescriptorTable.TableStart = m_HandleCache + CurrentOffset;
        RootDescriptorTable.TableSize = TableSize;

        CurrentOffset += TableSize;
    }

    m_MaxCachedDescriptors = CurrentOffset;

    ASSERT(m_MaxCachedDescriptors <= kMaxNumDescriptors, "Exceeded user-supplied maximum cache size");
}
