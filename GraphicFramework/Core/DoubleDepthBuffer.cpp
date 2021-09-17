#include "DoubleDepthBuffer.h"

void CDoubleDepthBuffer::Create2D(std::wstring Name, UINT Width, UINT Height, uint32_t NumMips, DXGI_FORMAT FrameFormat)
{
	GetFrontBuffer().Create2D(Name + L"(DoubleDepthBuffer0)", Width, Height, NumMips, FrameFormat);
	GetBackBuffer().Create2D(Name + L"(DoubleDepthBuffer1)", Width, Height, NumMips, FrameFormat);
}

void CDoubleDepthBuffer::SetClearDepth(float ClearDepthValue)
{
	GetFrontBuffer().SetClearDepth(ClearDepthValue);
	GetBackBuffer().SetClearDepth(ClearDepthValue);
}

void CDoubleDepthBuffer::SetClearStencil(uint8_t ClearStencilValue)
{
	GetFrontBuffer().SetClearStencil(ClearStencilValue);
	GetBackBuffer().SetClearStencil(ClearStencilValue);
}
