#include "LinearAllocator.h"

#include <thread>


using namespace std;

bool CLinearAllocator::s_IsInitialized = false;
CLinearAllocatorPagePool* CLinearAllocator::sm_pPageManager[2] = {nullptr, nullptr};

void CLinearAllocator::CleanupUsedPages( uint64_t FenceID )
{
	_ASSERTE(s_IsInitialized);

    if (m_CurPage == nullptr)
        return;

    m_RetiredPages.push_back(m_CurPage);
    m_CurPage = nullptr;
    m_CurOffset = 0;

    sm_pPageManager[m_AllocationType]->DiscardPages(FenceID, m_RetiredPages);
    m_RetiredPages.clear();

    sm_pPageManager[m_AllocationType]->FreeLargePages(FenceID, m_LargePageList);
    m_LargePageList.clear();
}

SDynAlloc CLinearAllocator::__AllocateLargePage(size_t SizeInBytes)
{
	_ASSERTE(s_IsInitialized);

    CLinearAllocationPage* OneOff = sm_pPageManager[m_AllocationType]->CreateNewPage(SizeInBytes);
    m_LargePageList.push_back(OneOff);

    SDynAlloc ret(*OneOff, 0, SizeInBytes);
    ret.DataPtr = OneOff->m_CpuVirtualAddress;
    ret.GpuAddress = OneOff->m_GpuVirtualAddress;

    return ret;
}

void CLinearAllocator::sInitialize(ID3D12Device* pDevice)
{
	_ASSERTE(pDevice);

	if (!s_IsInitialized)
	{
		for (int i = 0; i < 2; i++)
			sm_pPageManager[i] = new CLinearAllocatorPagePool(pDevice);
	}

	s_IsInitialized = true;
}

void CLinearAllocator::sShutdown()
{
	if (!s_IsInitialized) return;
	for (int i = 0; i < 2; i++) 
	{
		sm_pPageManager[i]->Destroy();
		delete sm_pPageManager[i];
	}
}

SDynAlloc CLinearAllocator::Allocate(size_t SizeInBytes, size_t Alignment)
{
	_ASSERTE(s_IsInitialized);

    const size_t AlignmentMask = Alignment - 1;

    // Assert that it's a power of two.
    ASSERT((AlignmentMask & Alignment) == 0);

    // Align the allocation
    const size_t AlignedSize = MathUtility::AlignUpWithMask(SizeInBytes, AlignmentMask);

    if (AlignedSize > m_PageSize)
        return __AllocateLargePage(AlignedSize);

    m_CurOffset = MathUtility::AlignUp(m_CurOffset, Alignment);

    if (m_CurOffset + AlignedSize > m_PageSize)
    {
        ASSERT(m_CurPage != nullptr);
        m_RetiredPages.push_back(m_CurPage);
        m_CurPage = nullptr;
    }

    if (m_CurPage == nullptr)
    {
        // Resource's(except for texture) default alignment is 64KB in DirectX12, so when we request a new page we do not have to align the m_CurOffset, just set m_CurOffset = 0 is ok.
		m_CurPage = sm_pPageManager[m_AllocationType]->RequestPage();
        m_CurOffset = 0;
    }

    SDynAlloc ret(*m_CurPage, m_CurOffset, AlignedSize);
    ret.DataPtr = (uint8_t*)m_CurPage->m_CpuVirtualAddress + m_CurOffset;
    ret.GpuAddress = m_CurPage->m_GpuVirtualAddress + m_CurOffset;

    m_CurOffset += AlignedSize;

    return ret;
}
