#pragma once

#include "GraphicsCoreBase.h"

class CCommandAllocatorPool
{
public:
    CCommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type);
    ~CCommandAllocatorPool();

    void Initialize(ID3D12Device* pDevice);
    void Shutdown();

    ID3D12CommandAllocator* RequestAllocator(uint64_t CompletedFenceValue);
    void DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator);

    inline size_t Size() { return m_AllocatorPool.size(); }

private:
    const D3D12_COMMAND_LIST_TYPE c_CommandListType;

	bool m_IsInitialized;
    ID3D12Device* m_pDevice;
    std::vector<ID3D12CommandAllocator*> m_AllocatorPool;
    std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_ReadyAllocators;
    std::mutex m_AllocatorMutex;
};
