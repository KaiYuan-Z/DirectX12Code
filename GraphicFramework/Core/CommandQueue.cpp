#include "GraphicsCoreBase.h"
#include "CommandQueue.h"

CCommandQueue::CCommandQueue(D3D12_COMMAND_LIST_TYPE Type) :
	m_IsInitialized(false),
	m_pDevice(nullptr),
	m_Type(Type),
	m_pCommandQueue(nullptr),
	m_pFence(nullptr),
	m_NextFenceValue((uint64_t)Type << 56 | 1),
	m_AllocatorPool(Type)
{
}

CCommandQueue::~CCommandQueue()
{
	Shutdown();
}

void CCommandQueue::Initialize(ID3D12Device* pDevice)
{
	if (!m_IsInitialized)
	{
		_ASSERT(pDevice != nullptr);
		_ASSERT(m_AllocatorPool.Size() == 0);

		m_pDevice = pDevice;

		D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
		QueueDesc.Type = m_Type;
		QueueDesc.NodeMask = 1;
		pDevice->CreateCommandQueue(&QueueDesc, MY_IID_PPV_ARGS(&m_pCommandQueue));
		_ASSERTE(m_pCommandQueue);
		m_pCommandQueue->SetName(L"CommandListManager::m_CommandQueue");

		ASSERT_SUCCEEDED(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, MY_IID_PPV_ARGS(&m_pFence)));
		_ASSERTE(m_pFence);
		m_pFence->SetName(L"CommandListManager::m_pFence");
		m_pFence->Signal((uint64_t)m_Type << 56);

		m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
		_ASSERT(m_FenceEventHandle != INVALID_HANDLE_VALUE);

		m_AllocatorPool.Initialize(pDevice);

		m_IsInitialized = true;
	}
}

void CCommandQueue::Shutdown()
{
	if (m_IsInitialized)
	{
		m_AllocatorPool.Shutdown();

		CloseHandle(m_FenceEventHandle);

		if (m_pFence != nullptr)
		{
			m_pFence->Release();
			m_pFence = nullptr;
		}
		
		if (m_pCommandQueue != nullptr)
		{
			m_pCommandQueue->Release();
			m_pCommandQueue = nullptr;
		}

		m_IsInitialized = false;
	}
}

void CCommandQueue::__CreateNewD3D12CommandList(D3D12_COMMAND_LIST_TYPE Type, ID3D12GraphicsCommandList** ppList, ID3D12CommandAllocator** ppAllocator)
{
	_ASSERTE(m_IsInitialized);
	*ppAllocator = __RequestAllocator();
	THROW_IF_FAILED(m_pDevice->CreateCommandList(1, Type, *ppAllocator, nullptr, MY_IID_PPV_ARGS(ppList)));
	(*ppList)->SetName(L"CommandList");
}

uint64_t CCommandQueue::__ExecuteCommandList(ID3D12CommandList* List)
{
	_ASSERTE(m_IsInitialized);

	ASSERT_SUCCEEDED(((ID3D12GraphicsCommandList*)List)->Close());

	// Kickoff the command list
	m_pCommandQueue->ExecuteCommandLists(1, &List);

	// And increment the fence value.  
	return IncrementFence();
}

uint64_t CCommandQueue::IncrementFence(void)
{
	std::lock_guard<std::mutex> LockGuard(m_FenceMutex);
	_ASSERTE(m_IsInitialized);
	m_pCommandQueue->Signal(m_pFence, m_NextFenceValue);
	return m_NextFenceValue++;
}

bool CCommandQueue::IsFenceComplete(uint64_t FenceValue)
{
	_ASSERTE(m_IsInitialized && m_pFence);
	return FenceValue <= m_pFence->GetCompletedValue();
}

uint64_t CCommandQueue::LastCompletedFenceValue()
{
	_ASSERTE(m_IsInitialized && m_pFence);
	return m_pFence->GetCompletedValue();
}

void CCommandQueue::StallForFence(CCommandQueue& Producer, uint64_t FenceValue)
{
	_ASSERTE(m_IsInitialized);
	m_pCommandQueue->Wait(Producer.m_pFence, FenceValue);
}

void CCommandQueue::StallForCommandQueue(CCommandQueue& Producer)
{
	_ASSERTE(m_IsInitialized);
	_ASSERT(Producer.m_NextFenceValue > 0);
	m_pCommandQueue->Wait(Producer.m_pFence, Producer.m_NextFenceValue - 1);
}

void CCommandQueue::WaitForFence(uint64_t FenceValue)
{
	_ASSERTE(m_IsInitialized && m_pFence);

	if (IsFenceComplete(FenceValue))
		return;

	// TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
	// wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
	// the fence can only have one event set on completion, then thread B has to wait for 
	// 100 before it knows 99 is ready.  Maybe insert sequential events?
	{
		std::lock_guard<std::mutex> LockGuard(m_EventMutex);

		m_pFence->SetEventOnCompletion(FenceValue, m_FenceEventHandle);
		WaitForSingleObject(m_FenceEventHandle, INFINITE);
	}
}

ID3D12CommandAllocator* CCommandQueue::__RequestAllocator()
{
	_ASSERTE(m_IsInitialized && m_pFence);
	uint64_t CompletedFence = m_pFence->GetCompletedValue();
	return m_AllocatorPool.RequestAllocator(CompletedFence);
}

void CCommandQueue::__DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator)
{
	_ASSERTE(m_IsInitialized);
	m_AllocatorPool.DiscardAllocator(FenceValue, Allocator);
}
