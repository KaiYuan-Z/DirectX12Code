#include "DescriptorAllocatorExplorer.h"


CDescriptorAllocatorExplorer::CDescriptorAllocatorExplorer() : m_IsInitialized(false), m_pDevice(nullptr), m_pDescriptorAllocators{ nullptr,nullptr,nullptr,nullptr }
{
}

CDescriptorAllocatorExplorer::~CDescriptorAllocatorExplorer()
{
	Shutdown();
}

void CDescriptorAllocatorExplorer::Initialize(ID3D12Device* pDevice)
{
	if (!m_IsInitialized)
	{
		_ASSERTE(pDevice);
		m_pDevice = pDevice;
		m_pDescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = new CDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_pDescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = new CDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		m_pDescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = new CDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_pDescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = new CDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_IsInitialized = true;
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE CDescriptorAllocatorExplorer::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count)
{
	_ASSERTE(m_IsInitialized);
	return m_pDescriptorAllocators[Type]->Allocate(Count);
}

void CDescriptorAllocatorExplorer::Shutdown(void)
{
	if (m_IsInitialized)
	{
		for (int type = 0; type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; type++)
		{
			if (m_pDescriptorAllocators)
			{
				delete m_pDescriptorAllocators[type];
				m_pDescriptorAllocators[type] = nullptr;
			}
		}

		m_IsInitialized = false;
	}
}
