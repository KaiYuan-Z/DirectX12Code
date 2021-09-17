#pragma once
#include "GpuBuffer.h"
#include "ByteAddressBuffer.h"

/*Structured Buffer

  A structured buffer is a buffer that contains elements of equal sizes.Use a structure with one or more member types to define an element.
  Here is a structure with three members.

  Use the following object types to access a structured buffer:
	StructuredBuffer is a read only structured buffer.
	RWStructuredBuffer is a read/write structured buffer.

  Aomic functions which implement interlocked operations are allowed on int and uint elements in an RWStructuredBuffer.
 */

class CStructuredBuffer : public CGpuBuffer
{
public:
	CStructuredBuffer() {};
	~CStructuredBuffer() {};

	virtual void Destroy(void) override
	{
		m_CounterBuffer.Destroy();
		CGpuBuffer::Destroy();
	}

	CByteAddressBuffer& GetCounterBuffer(void) { return m_CounterBuffer; }

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterSRV(CCommandList& CommandList);
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterUAV(CCommandList& CommandList);

protected:
	virtual void _CreateDerivedViews(void) override;
private:
	CByteAddressBuffer m_CounterBuffer;
};