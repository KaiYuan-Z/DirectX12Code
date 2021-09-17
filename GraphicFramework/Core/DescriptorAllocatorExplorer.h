#pragma once
#include "DescriptorAllocator.h"

class CDescriptorAllocatorExplorer
{
public:
	CDescriptorAllocatorExplorer();
	~CDescriptorAllocatorExplorer();
	void Initialize(ID3D12Device* pDevice);
	void Shutdown(void);
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1);

private:
	bool m_IsInitialized;
	ID3D12Device* m_pDevice;
	CDescriptorAllocator* m_pDescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};

