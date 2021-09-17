#pragma once
#include "CommandQueue.h"

enum ECommandQueueType
{
	kDirectCommandQueue = 0,
	kComputeCommandQueue = 1,
	kCopyCommandQueue = 2,

	kNumCommandQueueTypes
};

class CCommandQueueManager
{
public:
	CCommandQueueManager();
	~CCommandQueueManager();

	void Initialize(ID3D12Device* pDevice);
	void Shutdown();

	CCommandQueue& GetCommandQueue(ECommandQueueType Type = kDirectCommandQueue)
	{
		_ASSERTE(Type != kNumCommandQueueTypes);
		return m_CommandQueues[Type];
	}

	CCommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE Type)
	{
		switch (Type)
		{
		case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_CommandQueues[kComputeCommandQueue];
		case D3D12_COMMAND_LIST_TYPE_COPY: return m_CommandQueues[kCopyCommandQueue];
		default: return m_CommandQueues[kDirectCommandQueue];
		}
	}

private:
	bool m_IsInitialized;
	ID3D12Device* m_pDevice;
	CCommandQueue m_CommandQueues[kNumCommandQueueTypes];
};

