
#include "GraphicsCoreBase.h"
#include "CommandAllocatorPool.h"

CCommandAllocatorPool::CCommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type) :
    c_CommandListType(Type),
	m_IsInitialized(false),
	m_pDevice(nullptr)
{
}

CCommandAllocatorPool::~CCommandAllocatorPool()
{
    Shutdown();
}

void CCommandAllocatorPool::Initialize(ID3D12Device* pDevice)
{
	if (!m_IsInitialized)
	{
		_ASSERTE(pDevice);
		m_pDevice = pDevice;
		m_IsInitialized = true;
	}
	
}

void CCommandAllocatorPool::Shutdown()
{
	if (m_IsInitialized)
	{
		for (size_t i = 0; i < m_AllocatorPool.size(); ++i)
			m_AllocatorPool[i]->Release();

		m_AllocatorPool.clear();

		m_IsInitialized = false;
	}
}

ID3D12CommandAllocator * CCommandAllocatorPool::RequestAllocator(uint64_t CompletedFenceValue)
{
    std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);

	_ASSERTE(m_IsInitialized);

    ID3D12CommandAllocator* pAllocator = nullptr;

    if (!m_ReadyAllocators.empty())
    {
        std::pair<uint64_t, ID3D12CommandAllocator*>& AllocatorPair = m_ReadyAllocators.front();

        if (AllocatorPair.first <= CompletedFenceValue)
        {
            pAllocator = AllocatorPair.second;
            ASSERT_SUCCEEDED(pAllocator->Reset());
            m_ReadyAllocators.pop();
        }
    }

    // If no allocator's were ready to be reused, create a new one
    if (pAllocator == nullptr)
    {
        ASSERT_SUCCEEDED(m_pDevice->CreateCommandAllocator(c_CommandListType, MY_IID_PPV_ARGS(&pAllocator)));
        wchar_t AllocatorName[32];
        swprintf(AllocatorName, 32, L"CommandAllocator %zu", m_AllocatorPool.size());
        pAllocator->SetName(AllocatorName);
        m_AllocatorPool.push_back(pAllocator);
    }

    return pAllocator;
}

void CCommandAllocatorPool::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator * Allocator)
{
    std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);

	_ASSERTE(m_IsInitialized);

    // That fence value indicates we are free to reset the allocator
    m_ReadyAllocators.push(std::make_pair(FenceValue, Allocator));
}
