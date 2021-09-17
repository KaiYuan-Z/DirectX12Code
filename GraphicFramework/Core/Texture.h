#pragma once
#include "GpuResource.h"

class CTexture : public CGpuResource
{
	friend class CCommandList;

public:

	CTexture() 
	{
		m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; 
	}
	CTexture(D3D12_CPU_DESCRIPTOR_HANDLE Handle) : m_hCpuDescriptorHandle(Handle) {}

	// Create a 1-level 2D texture
	void Create2D(size_t Pitch, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData);
	void Create2D(size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData)
	{
		Create2D(Width, Width, Height, Format, InitData);
	}

	void CreateTGAFromMemory(const void* MemBuffer, size_t FileSize, bool sRGB);
	bool CreateDDSFromMemory(const void* MemBuffer, size_t FileSize, bool sRGB);
	void CreatePIXImageFromMemory(const void* MemBuffer, size_t FileSize);

	virtual void Destroy() override
	{
		CGpuResource::Destroy();
		m_hCpuDescriptorHandle.ptr = 0;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_hCpuDescriptorHandle; }

	bool operator!() { return m_hCpuDescriptorHandle.ptr == 0; }

protected:

	D3D12_CPU_DESCRIPTOR_HANDLE m_hCpuDescriptorHandle;
};