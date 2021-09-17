#pragma once

#include "DescriptorHandle.h"
#include "RootSignature.h"
#include <vector>
#include <queue>
#include <mutex>

class CCommandQueueManager;
class CCommandList;

// This class is a linear allocation system for dynamically generated descriptor tables.  It internally caches
// CPU descriptor handles so that when not enough space is available in the current heap, necessary descriptors
// can be re-copied to the new heap.
// Behavior in command list:
// (1) CommandList->SetRootSignature(...): Call ParseGraphicsRootSignature(...), reset SDescriptorHandleCache.
// (2) CommandList->SetDynamicDescriptor(...): Call StageDescriptorHandles(...), mark used root index and store handles in a temporary cache.
// (3) CommandList->DrawInstanced(...)/ExecuteIndirect(...)/CommitDynamicDescriptorsManually(...): First, call SetDescriptorHeap(...); Second, 
//   call __CopyAndBindStagedTables(...); Note:If there is not enough space in current heap£¬we will request a new one, and copy new handles to it.
//   If current heap has enough space, the new handles will be appenned after previous hanles.If the dynamic descriptor is setted, the heap setted 
//   by user will be overwritten by the dynamic heap.
// (4) CommandList->ClearUAV(...): Just copy a handle to a GPU visible heap, and it does not need to SetDescriptorHeap(...). This function does not change
//    temporary cache, so it does not affect other handles. 
// (5) CommandList->__Finish(...): Retire used heaps, clear cache. So, it guarantees that the command list will alway have a new empty heap at every beginning.

class CDynamicDescriptorHeap
{
public:
	static void sInitialize(ID3D12Device* pDevice);
	static void sShutdown();

    CDynamicDescriptorHeap(CCommandList& CommandList, D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
    ~CDynamicDescriptorHeap();
 
	void SetTemporaryUserHeap(ID3D12DescriptorHeap* HeapPtr, UINT HeapSize, UINT StartOffset);

    void CleanupUsedHeaps( uint64_t fenceValue );

    // Copy multiple handles into the cache area reserved for the specified root parameter.
    void SetGraphicsDescriptorHandles( UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] )
    {
        m_GraphicsHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
    }

    void SetComputeDescriptorHandles( UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] )
    {
        m_ComputeHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
    }

    // Bypass the cache and upload directly to the shader-visible heap, only be used in ClearUAV().
    D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect( D3D12_CPU_DESCRIPTOR_HANDLE Handles );

    // Deduce cache layout needed to support the descriptor tables needed by the root signature.
    void ParseGraphicsRootSignature( const CRootSignature& RootSig )
    {
        m_GraphicsHandleCache.ParseRootSignature(m_DescriptorType, RootSig);
    }

    void ParseComputeRootSignature( const CRootSignature& RootSig )
    {
        m_ComputeHandleCache.ParseRootSignature(m_DescriptorType, RootSig);
    }

    // Upload any new descriptors in the cache to the shader-visible heap.
    inline void CommitGraphicsRootDescriptorTables( ID3D12GraphicsCommandList* CmdList )
    {
        if (m_GraphicsHandleCache.m_StaleRootParamsBitMap != 0)
            __CopyAndBindStagedTables(m_GraphicsHandleCache, CmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
    }

    inline void CommitComputeRootDescriptorTables( ID3D12GraphicsCommandList* CmdList )
    {
        if (m_ComputeHandleCache.m_StaleRootParamsBitMap != 0)
            __CopyAndBindStagedTables(m_ComputeHandleCache, CmdList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
    }

private:

    // Static members
	static bool sm_IsInitialized;
	static ID3D12Device* sm_pDevice;
    static const uint32_t kNumDescriptorsPerHeap = 1024;
    static std::mutex sm_Mutex;
    static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> sm_DescriptorHeapPool[2];
    static std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>> sm_RetiredDescriptorHeaps[2];
    static std::queue<ID3D12DescriptorHeap*> sm_AvailableDescriptorHeaps[2];

    // Static methods
    static ID3D12DescriptorHeap* __sRequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, CCommandQueueManager* pCommandQueueManager);
    static void __sDiscardDescriptorHeaps( D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValueForReset, const std::vector<ID3D12DescriptorHeap*>& UsedHeaps );

    // Non-static members
    CCommandList& m_OwningCommandList;
    ID3D12DescriptorHeap* m_CurrentHeapPtr;
	ID3D12DescriptorHeap* m_UserHeapPtr;
    const D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorType;
    uint32_t m_DescriptorSize;
    uint32_t m_CurrentOffset;
	uint32_t m_UserHeapSize;
	uint32_t m_UserHeapStartOffset;
    CDescriptorHandle m_FirstDescriptor;
	CDescriptorHandle m_UserHeapFirstDescriptor;
    std::vector<ID3D12DescriptorHeap*> m_RetiredHeaps;

    // Describes a descriptor table entry:  a region of the handle cache and which handles have been set
    struct SDescriptorTableCache
    {
        SDescriptorTableCache() : AssignedHandlesBitMap(0) {}
        uint32_t AssignedHandlesBitMap;// Each bit represent a handle
        D3D12_CPU_DESCRIPTOR_HANDLE* TableStart;
        uint32_t TableSize;
    };

    struct SDescriptorHandleCache
    {
        SDescriptorHandleCache()
        {
            ClearCache();
        }

        void ClearCache()
        {
            m_RootDescriptorTablesBitMap = 0;
            m_MaxCachedDescriptors = 0;
        }

        uint32_t m_RootDescriptorTablesBitMap;// One bit is set for root signature that are descriptor tables
        uint32_t m_StaleRootParamsBitMap;
        uint32_t m_MaxCachedDescriptors;

        static const uint32_t kMaxNumDescriptors = 512;
        static const uint32_t kMaxNumDescriptorTables = 16;

        uint32_t ComputeStagedSize();
        void CopyAndBindStaleTables( D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t DescriptorSize, CDescriptorHandle DestHandleStart, ID3D12GraphicsCommandList* CmdList,
            void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

        SDescriptorTableCache m_RootDescriptorTable[kMaxNumDescriptorTables];
        D3D12_CPU_DESCRIPTOR_HANDLE m_HandleCache[kMaxNumDescriptors];

        void StageDescriptorHandles( UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] );
        void ParseRootSignature( D3D12_DESCRIPTOR_HEAP_TYPE Type, const CRootSignature& RootSig );
    };

    SDescriptorHandleCache m_GraphicsHandleCache;
    SDescriptorHandleCache m_ComputeHandleCache;

    bool __HasSpace( uint32_t Count )
    {
        return (m_CurrentHeapPtr != nullptr && m_CurrentOffset + Count <= kNumDescriptorsPerHeap);
    }

    void __RetireCurrentHeap(void);
    void __RetireUsedHeaps( uint64_t fenceValue );
    ID3D12DescriptorHeap* __GetHeapPointer();

    CDescriptorHandle __Allocate( UINT Count )
    {
        CDescriptorHandle ret = m_FirstDescriptor + m_CurrentOffset * m_DescriptorSize;
        m_CurrentOffset += Count;
        return ret;
    }

    void __CopyAndBindStagedTables( SDescriptorHandleCache& HandleCache, ID3D12GraphicsCommandList* CmdList,
        void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE) );

	void __RetireUserHeap()
	{
		m_UserHeapPtr = nullptr;
		m_UserHeapSize = 0;
		m_UserHeapStartOffset = 0;
	}
};
