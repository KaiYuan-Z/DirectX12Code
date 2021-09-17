#pragma once
#include "GpuBuffer.h"

/*Byte Address Buffer
A byte address buffer is a buffer whose contents are addressable by a byte offset. Normally, the contents of a buffer are indexed per element using a stride for 
each element (S) and the element number (N) as given by S*N. A byte address buffer, which can also be called a raw buffer, uses a byte value offset from the beginning
of the buffer to access data. The byte value must be a multiple of four so that it is DWORD aligned. If any other value is provided, behavior is undefined.

Shader model 5 introduces objects for accessing a read-only byte address buffer as well as a read-write byte address buffer. The contents of a byte address buffer is 
designed to be a 32-bit unsigned integer; if the value in the buffer is not really an unsigned integer, use a function such as asfloat to read it.*/

class CByteAddressBuffer : public CGpuBuffer
{
public:
	CByteAddressBuffer() {};
	~CByteAddressBuffer() {};

protected:
	virtual void _CreateDerivedViews(void) override;
};