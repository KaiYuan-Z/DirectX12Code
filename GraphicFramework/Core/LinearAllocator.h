// Description:  This is a dynamic graphics memory allocator for DX12.  It's designed to work in concert
// with the CCommandList class and to do so in a thread-safe manner.  There may be many command lists,
// each with its own linear allocators.  They act as windows into a global memory pool by reserving a
// command list-local memory page.  Requesting a new page is done in a thread-safe manner by guarding accesses
// with a mutex lock.
//
// When a command list is finished, it will receive a fence ID that indicates when it's safe to reclaim
// used resources.  The CleanupUsedPages() method must be invoked at this time so that the used pages can be
// scheduled for reuse after the fence has cleared.

#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include "GraphicsCoreBase.h"
#include "GpuResource.h"
#include "LinearAllocatorPagePool.h"

// Constant blocks must be multiples of 16 constants @ 16 bytes each
#define DEFAULT_ALIGN 256

// Various types of allocations may contain NULL pointers.  Check before dereferencing if you are unsure.
struct SDynAlloc
{
    SDynAlloc(CGpuResource& BaseResource, size_t ThisOffset, size_t ThisSize)
        : Buffer(BaseResource), Offset(ThisOffset), Size(ThisSize) {}

    CGpuResource& Buffer;	// The D3D buffer associated with this memory.
    size_t Offset;			// Offset from start of buffer resource
    size_t Size;			// Reserved size of this allocation
    void* DataPtr;			// The CPU-writeable address
    D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;	// The GPU-visible address
};

class CLinearAllocator
{
public:

    CLinearAllocator(ELinearAllocatorType Type) : m_AllocationType(Type), m_PageSize(0), m_CurOffset(~(size_t)0), m_CurPage(nullptr)
    {
        ASSERT(Type > kInvalidAllocator && Type < kNumAllocatorTypes);
        m_PageSize = (Type == kGpuExclusive ? kGpuAllocatorPageSize : kCpuAllocatorPageSize);
    }

	static void sInitialize(ID3D12Device* pDevice);
	static void sShutdown();

    SDynAlloc Allocate( size_t SizeInBytes, size_t Alignment = DEFAULT_ALIGN );

    void CleanupUsedPages( uint64_t FenceID );

private:

    SDynAlloc __AllocateLargePage( size_t SizeInBytes );

	static bool s_IsInitialized;
    static CLinearAllocatorPagePool* sm_pPageManager[2];

    ELinearAllocatorType m_AllocationType;
    size_t m_PageSize;
    size_t m_CurOffset;
    CLinearAllocationPage* m_CurPage;
    std::vector<CLinearAllocationPage*> m_RetiredPages;
    std::vector<CLinearAllocationPage*> m_LargePageList;
};
