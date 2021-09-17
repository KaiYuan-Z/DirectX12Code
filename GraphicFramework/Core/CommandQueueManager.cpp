#include "CommandQueueManager.h"
#include "D3D12Base.h"

CCommandQueueManager::CCommandQueueManager():
	m_pDevice(nullptr),
	m_IsInitialized(false), 
	m_CommandQueues{ D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12_COMMAND_LIST_TYPE_COPY }
{
}

CCommandQueueManager::~CCommandQueueManager()
{
	Shutdown();
}

void CCommandQueueManager::Initialize(ID3D12Device * pDevice)
{
	if (!m_IsInitialized)
	{
		_ASSERTE(pDevice != nullptr);

		m_pDevice = pDevice;

		for (int type = 0; type < kNumCommandQueueTypes; type++)
			m_CommandQueues[type].Initialize(pDevice);

		m_IsInitialized = true;
	}
}

void CCommandQueueManager::Shutdown()
{
	if (m_IsInitialized)
	{
		for (int type = 0; type < kNumCommandQueueTypes; type++)
			m_CommandQueues[type].Shutdown();

		m_IsInitialized = false;
	}
}
