#pragma once
#include "UploadBuffer.h"
#include "GraphicsCore.h"

class CTripleUploadBuffer
{
public:
	CTripleUploadBuffer() {}
	~CTripleUploadBuffer() 
	{
		for(int i = 0; i < 3; i++)
		{
			m_UploadBuffers[i].Unmap();
			m_MappedDataSet[i] = nullptr;
		}
	}

	void Create(const std::wstring& Name, uint32_t NumElements, uint32_t ElementSize, const void* initialData = nullptr)
	{
		for (int i = 0; i < 3; i++)
		{
			m_UploadBuffers[i].Create(Name + MakeWStr("(TripleUploadBuffer)" + i), NumElements, ElementSize);

			if (initialData)
			{
				size_t BufferSize = NumElements * ElementSize;
				m_MappedDataSet[i] = m_UploadBuffers[i].Map();
				memcpy(m_MappedDataSet[i], initialData, BufferSize);
			}
		}
	}

	void MoveToNextBuffer()
	{
		CCommandQueue& CommandQueue = GraphicsCore::GetCommandQueue();
		m_UploadBufferFences[m_CurrentBufferIndex] = CommandQueue.IncrementFence();

		m_CurrentBufferIndex = m_CurrentBufferIndex == 2? 0 : m_CurrentBufferIndex++;
		if (m_UploadBufferFences[m_CurrentBufferIndex] > 0) CommandQueue.WaitForFence(m_UploadBufferFences[m_CurrentBufferIndex]);
	}

	CUploadBuffer& GetCurrentBuffer() { return m_UploadBuffers[m_CurrentBufferIndex]; }
	void* GetCurrentBufferMappedData() { return m_MappedDataSet[m_CurrentBufferIndex]; }
	UINT GetCurrentBackBufferIndex() { return m_CurrentBufferIndex; }
	UINT GetPreviousBackBufferIndex() { return m_CurrentBufferIndex == 0 ? 2 : m_CurrentBufferIndex - 1; }

private:
	UINT m_CurrentBufferIndex = 0;
	uint64_t m_UploadBufferFences[3] = { 0 };
	CUploadBuffer m_UploadBuffers[3];
	void* m_MappedDataSet[3];
};