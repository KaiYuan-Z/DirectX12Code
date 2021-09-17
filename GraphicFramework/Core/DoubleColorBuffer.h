#pragma once
#include "DoubleBuffer.h"
#include "ColorBuffer.h"

class CDoubleColorBuffer : public TDoubleBuffer<CColorBuffer>
{
public:

	CDoubleColorBuffer() {}
	~CDoubleColorBuffer() {}

	void Create2D(std::wstring Name, UINT Width, UINT Height, uint32_t NumMips, DXGI_FORMAT FrameFormat);
};