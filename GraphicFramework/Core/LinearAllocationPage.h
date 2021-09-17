#pragma once
#include "GpuResource.h"

class CLinearAllocationPage : public CGpuResource
{
public:
	CLinearAllocationPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES Usage) : CGpuResource()
	{
		m_pResource.Attach(pResource);
		m_UsageState = Usage;
		m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();
		m_pResource->Map(0, nullptr, &m_CpuVirtualAddress);
	}

	~CLinearAllocationPage()
	{
		Unmap();
	}

	void Map(void)
	{
		if (m_CpuVirtualAddress == nullptr)
		{
			m_pResource->Map(0, nullptr, &m_CpuVirtualAddress);
		}
	}

	void Unmap(void)
	{
		if (m_CpuVirtualAddress != nullptr)
		{
			m_pResource->Unmap(0, nullptr);
			m_CpuVirtualAddress = nullptr;
		}
	}

	void* m_CpuVirtualAddress;
	D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
};

