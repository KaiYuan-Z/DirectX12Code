#pragma once
#include "GpuBuffer.h"

class CUploadBuffer : public CGpuBuffer
{
public:
	CUploadBuffer() : m_pMappedData(nullptr) {}
	~CUploadBuffer() {}

	void Create(const std::wstring& Name, uint32_t NumElements, uint32_t ElementSize);

	void* Map(void);
	void Unmap(void);

protected:
	void _CreateDerivedViews(void);

private:
	void* m_pMappedData;
};

