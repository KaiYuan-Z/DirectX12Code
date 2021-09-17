#pragma once
#include "ByteAddressBuffer.h"
#include "GraphicsCore.h"

// Used in partical effect.
class CIndirectArgsBuffer : public CByteAddressBuffer
{
public:
	CIndirectArgsBuffer() {};
	~CIndirectArgsBuffer() {};
};