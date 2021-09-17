#pragma once
#include <vector>
#include <mutex>
#include <queue>
#include "CommandList.h"
#include "CommandQueueManager.h"

class CCommandListManager
{
public:
	CCommandListManager() : m_IsInitialized(false), m_pDevice(nullptr), m_pCommandQueueManager(nullptr){}
	~CCommandListManager() { Shutdown(); }

	void Initialize(ID3D12Device* pDevice, CCommandQueueManager* pCommandQueueManager);
	void Shutdown();

	CCommandList* AllocateCommandList(D3D12_COMMAND_LIST_TYPE Type);
	void FreeCommandList(CCommandList* pUsedCommandList);

private:
	bool m_IsInitialized;
	ID3D12Device* m_pDevice;
	CCommandQueueManager* m_pCommandQueueManager;
	std::vector<std::unique_ptr<CCommandList> > m_CommandListPool[4];
	std::queue<CCommandList*> m_AvailableCommandLists[4];
	std::mutex m_CommandListAllocationMutex;
};
