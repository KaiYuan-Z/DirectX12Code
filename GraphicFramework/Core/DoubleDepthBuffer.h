#pragma once
#pragma once
#include "DoubleBuffer.h"
#include "DepthBuffer.h"

class CDoubleDepthBuffer : public TDoubleBuffer<CDepthBuffer>
{
public:

	CDoubleDepthBuffer() {}
	~CDoubleDepthBuffer() {}

	void Create2D(std::wstring Name, UINT Width, UINT Height, uint32_t NumMips, DXGI_FORMAT FrameFormat);

	void SetClearDepth(float ClearDepthValue);
	void SetClearStencil(uint8_t ClearStencilValue);
};