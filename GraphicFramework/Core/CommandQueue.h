#pragma once
#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>
#include "CommandAllocatorPool.h"

class CCommandQueue
{
	friend class CommandListManager;
	friend class CCommandList;

public:
	CCommandQueue(D3D12_COMMAND_LIST_TYPE Type);
	~CCommandQueue();

	void Initialize(ID3D12Device* pDevice);
	void Shutdown();

	inline bool IsReady()
	{
		return m_IsInitialized;
	}

	uint64_t IncrementFence(void);
	bool IsFenceComplete(uint64_t FenceValue);
	uint64_t LastCompletedFenceValue(void);
	void StallForFence(CCommandQueue& Producer, uint64_t FenceValue);
	void StallForCommandQueue(CCommandQueue& Producer);
	void WaitForFence(uint64_t FenceValue);
	void WaitForIdle(void) { WaitForFence(IncrementFence()); }
	
	ID3D12CommandQueue* GetCommandQueue() { return m_pCommandQueue; }

	uint64_t GetNextFenceValue() { return m_NextFenceValue; }

private:
	void __CreateNewD3D12CommandList(D3D12_COMMAND_LIST_TYPE Type, ID3D12GraphicsCommandList** ppList, ID3D12CommandAllocator** ppAllocator);
	uint64_t __ExecuteCommandList(ID3D12CommandList* List);
	ID3D12CommandAllocator* __RequestAllocator(void);
	void __DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator* Allocator);

	bool m_IsInitialized;

	ID3D12Device* m_pDevice;

	ID3D12CommandQueue* m_pCommandQueue;

	const D3D12_COMMAND_LIST_TYPE m_Type;

	CCommandAllocatorPool m_AllocatorPool;

	std::mutex m_FenceMutex;
	std::mutex m_EventMutex;
	ID3D12Fence* m_pFence;
	uint64_t m_NextFenceValue;
	HANDLE m_FenceEventHandle;
};
