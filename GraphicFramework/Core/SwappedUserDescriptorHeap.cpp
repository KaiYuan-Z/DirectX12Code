#include "SwappedUserDescriptorHeap.h"
#include "GraphicsCore.h"


CSwappedUserDescriptorHeap::CSwappedUserDescriptorHeap() : CSwappedUserDescriptorHeap(L"")
{
}

CSwappedUserDescriptorHeap::CSwappedUserDescriptorHeap(const std::wstring& name) : m_Name(name), m_HeapIndex(0)
{
}

CSwappedUserDescriptorHeap::~CSwappedUserDescriptorHeap()
{
}

void CSwappedUserDescriptorHeap::Create(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxDescCount, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>* pInitHandleSet)
{
	_ASSERTE(pDevice);
	m_HeapIndex = 0;
	for (size_t heapIndex = 0; heapIndex < 3; heapIndex++)
	{
		m_SwappedHeaps[heapIndex].Create(pDevice, Type, MaxDescCount, m_Name);
		m_SwappedHeapFences[heapIndex] = 0;

		if (pInitHandleSet)
		{
			for (size_t handleIndex = 0; handleIndex < pInitHandleSet->size(); handleIndex++)
			{
				m_SwappedHeaps[heapIndex].AddHandleToHeap((*pInitHandleSet)[handleIndex], 1);
			}
		}
	}
}

void CSwappedUserDescriptorHeap::SwapUserHeap(uint64_t fenceValue)
{
	m_SwappedHeapFences[m_HeapIndex] = fenceValue;
	m_HeapIndex = (m_HeapIndex + 1) % 3;
	GraphicsCore::WaitForFence(m_SwappedHeapFences[m_HeapIndex]);
}

