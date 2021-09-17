#pragma once
#include "GpuBuffer.h"

class CTypedBuffer : public CGpuBuffer
{
public:
	CTypedBuffer(DXGI_FORMAT Format) : m_DataFormat(Format) {}
	~CTypedBuffer() {};

protected:
	DXGI_FORMAT m_DataFormat;

	virtual void _CreateDerivedViews(void) override;
};

