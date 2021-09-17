#include "DescriptorAllocator.h"


//
// DescriptorAllocator implementation
//

bool CDescriptorAllocator::sm_IsInitialized = false;
ID3D12Device* CDescriptorAllocator::sm_pDevice = nullptr;
std::mutex CDescriptorAllocator::sm_AllocationMutex;
std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> CDescriptorAllocator::sm_DescriptorHeapPool;

void CDescriptorAllocator::sInitialize(ID3D12Device* pDevice)
{
	if (!sm_IsInitialized)
	{
		_ASSERTE(pDevice);
		sm_pDevice = pDevice;
		sm_IsInitialized = true;
	}
}

void CDescriptorAllocator::sShutdown()
{
	if (sm_IsInitialized)
	{
		sm_DescriptorHeapPool.clear();
		sm_IsInitialized = false;
	}
}

ID3D12DescriptorHeap* CDescriptorAllocator::_sRequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	std::lock_guard<std::mutex> LockGuard(sm_AllocationMutex);

	_ASSERTE(sm_IsInitialized); 

	D3D12_DESCRIPTOR_HEAP_DESC Desc;
	Desc.Type = Type;
	Desc.NumDescriptors = sm_NumDescriptorsPerHeap;
	Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	Desc.NodeMask = 1;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pHeap;
	ASSERT_SUCCEEDED(sm_pDevice->CreateDescriptorHeap(&Desc, MY_IID_PPV_ARGS(&pHeap)));
	sm_DescriptorHeapPool.emplace_back(pHeap);
	return pHeap.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE CDescriptorAllocator::Allocate(uint32_t Count)
{
	if (m_pCurrentHeap == nullptr || m_RemainingFreeHandles < Count)
	{
		m_pCurrentHeap = _sRequestNewHeap(m_Type);
		m_CurrentHandle = m_pCurrentHeap->GetCPUDescriptorHandleForHeapStart();
		m_RemainingFreeHandles = sm_NumDescriptorsPerHeap;

		if (m_DescriptorSize == 0)
			m_DescriptorSize = sm_pDevice->GetDescriptorHandleIncrementSize(m_Type);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE ret = m_CurrentHandle;
	m_CurrentHandle.ptr += Count * m_DescriptorSize;
	m_RemainingFreeHandles -= Count;
	return ret;
}
