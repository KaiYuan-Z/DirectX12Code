#include "CommandListManager.h"


void CCommandListManager::Initialize(ID3D12Device* pDevice, CCommandQueueManager* pCommandQueueManager)
{
	if (!m_IsInitialized)
	{
		_ASSERTE(pDevice && pCommandQueueManager);

		m_pDevice = pDevice;
		m_pCommandQueueManager = pCommandQueueManager;
		m_IsInitialized = true;
	}
}

void CCommandListManager::Shutdown()
{
	if (m_IsInitialized)
	{
		for (uint32_t i = 0; i < 4; ++i)
			m_CommandListPool[i].clear();

		m_IsInitialized = false;
	}
}

CCommandList* CCommandListManager::AllocateCommandList(D3D12_COMMAND_LIST_TYPE Type)
{
	_ASSERTE(m_IsInitialized);
	
	std::lock_guard<std::mutex> LockGuard(m_CommandListAllocationMutex);

	auto& AvailableCommandLists = m_AvailableCommandLists[Type];

	CCommandList* ret = nullptr;
	if (AvailableCommandLists.empty())
	{
		ret = new CCommandList(Type);
		m_CommandListPool[Type].emplace_back(ret);
		ret->Initialize(m_pDevice, m_pCommandQueueManager);
	}
	else
	{
		ret = AvailableCommandLists.front();
		AvailableCommandLists.pop();
		ret->__Reset();
	}
	_ASSERTE(ret != nullptr);

	_ASSERTE(ret->m_Type == Type);

	return ret;
}

void CCommandListManager::FreeCommandList(CCommandList* pUsedCommandList)
{
	_ASSERTE(m_IsInitialized && pUsedCommandList != nullptr);

	std::lock_guard<std::mutex> LockGuard(m_CommandListAllocationMutex);
	m_AvailableCommandLists[pUsedCommandList->m_Type].push(pUsedCommandList);
}