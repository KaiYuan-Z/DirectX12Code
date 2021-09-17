#pragma once
#include "UserDescriptorHeap.h"

class CSwappedUserDescriptorHeap
{
public:
	CSwappedUserDescriptorHeap();
	CSwappedUserDescriptorHeap(const std::wstring& name);
	~CSwappedUserDescriptorHeap();

	void Create(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxDescCount, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>* pInitHandleSet = nullptr);

	CUserDescriptorHeap& GetCurUserHeap() { return m_SwappedHeaps[m_HeapIndex]; }

	void SwapUserHeap(uint64_t fenceValue);

private:
	std::wstring m_Name;
	UINT m_HeapIndex;
	CUserDescriptorHeap m_SwappedHeaps[3];
	uint64_t m_SwappedHeapFences[3];
};

