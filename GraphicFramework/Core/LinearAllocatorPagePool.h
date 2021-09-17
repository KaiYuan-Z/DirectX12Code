#pragma once
#include <queue>
#include <vector>
#include <mutex>
#include "LinearAllocationPage.h"

enum ELinearAllocatorType
{
	kInvalidAllocator = -1,

	kGpuExclusive = 0,		// DEFAULT   GPU-writeable (via UAV)
	kCpuWritable = 1,		// UPLOAD CPU-writeable (but write combined)

	kNumAllocatorTypes
};

enum
{
	kGpuAllocatorPageSize = 0x10000,	// 64K
	kCpuAllocatorPageSize = 0x200000	// 2MB
};

class CLinearAllocatorPagePool
{
public:

	CLinearAllocatorPagePool(ID3D12Device* pDevice);
	CLinearAllocationPage* RequestPage(void);
	CLinearAllocationPage* CreateNewPage(size_t PageSize = 0);

	// Discarded pages will get recycled.  This is for fixed size pages.
	void DiscardPages(uint64_t FenceID, const std::vector<CLinearAllocationPage*>& Pages);

	// Freed pages will be destroyed once their fence has passed.  This is for single-use,
	// "large" pages.
	void FreeLargePages(uint64_t FenceID, const std::vector<CLinearAllocationPage*>& Pages);

	void Destroy(void) { m_PagePool.clear(); }

private:

	static ELinearAllocatorType sm_AutoType;

	ELinearAllocatorType m_AllocationType;
	std::vector<std::unique_ptr<CLinearAllocationPage> > m_PagePool;
	std::queue<std::pair<uint64_t, CLinearAllocationPage*> > m_RetiredPages;
	std::queue<std::pair<uint64_t, CLinearAllocationPage*> > m_DeletionQueue;
	std::queue<CLinearAllocationPage*> m_AvailablePages;
	std::mutex m_Mutex;

	ID3D12Device* m_pDevice;
};

