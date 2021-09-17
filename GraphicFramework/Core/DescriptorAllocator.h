#pragma once
#include "GraphicsCoreBase.h"

// This is an unbounded resource descriptor allocator.  It is intended to provide space for CPU-visible resource descriptors
// as resources are created.  For those that need to be made shader-visible, they will need to be copied to a UserDescriptorHeap
// or a DynamicDescriptorHeap.
class CDescriptorAllocator
{
public:

	static void sInitialize(ID3D12Device* pDevice);
	static void sShutdown();

	CDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE Type) : m_Type(Type), m_pCurrentHeap(nullptr) {}
	D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32_t Count);

protected:

	D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	ID3D12DescriptorHeap* m_pCurrentHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentHandle;
	uint32_t m_DescriptorSize;
	uint32_t m_RemainingFreeHandles;

	static ID3D12DescriptorHeap* _sRequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type);

	static bool sm_IsInitialized;
	static ID3D12Device* sm_pDevice;
	static const uint32_t sm_NumDescriptorsPerHeap = 256;
	static std::mutex sm_AllocationMutex;
	static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> sm_DescriptorHeapPool;
};

