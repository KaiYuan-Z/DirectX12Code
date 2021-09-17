#pragma once
#include "GpuBuffer.h"

class CConstantBuffer : public CGpuBuffer
{
public:
	CConstantBuffer() {}
	~CConstantBuffer() {}

protected:
	void _CreateDerivedViews(void) {}
};

