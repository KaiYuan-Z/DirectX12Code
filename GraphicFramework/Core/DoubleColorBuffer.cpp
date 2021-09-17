#include "DoubleColorBuffer.h"

void CDoubleColorBuffer::Create2D(std::wstring Name, UINT Width, UINT Height, uint32_t NumMips, DXGI_FORMAT FrameFormat)
{
	GetFrontBuffer().Create2D(Name + L"(DoubleColorBuffer0)", Width, Height, NumMips, FrameFormat);
	GetBackBuffer().Create2D(Name + L"(DoubleColorBuffer1)", Width, Height, NumMips, FrameFormat);
}
