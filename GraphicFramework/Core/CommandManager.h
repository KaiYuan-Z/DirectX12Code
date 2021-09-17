#pragma once
#include "CommandQueueManager.h"
#include "CommandListManager.h"

class CCommandManager
{
public:
	CCommandManager();
	~CCommandManager();

	void Initialize(ID3D12Device* pDevice);
	void Shutdown();

	CCommandList* BeginCommandList(const std::wstring& ID = L"");
	CGraphicsCommandList* BeginGraphicsCommandList(const std::wstring& ID = L"");
	CComputeCommandList* BeginComputeCommandList(const std::wstring& ID = L"", bool Async = false);
	uint64_t FinishCommandList(CCommandList*& pCommandList, bool WaitForCompletion = false);
	uint64_t FinishCommandList(CGraphicsCommandList*& pCommandList, bool WaitForCompletion = false);
	uint64_t FinishCommandList(CComputeCommandList*& pCommandList, bool WaitForCompletion = false);

	void InitializeTexture(CGpuResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
	void InitializeBuffer(CGpuResource& Dest, const void* pBufferData, size_t NumBytes, size_t Offset = 0);
	void InitializeTextureArraySlice(CGpuResource& Dest, UINT SliceIndex, CGpuResource& Src);
	void ReadbackTexture2D(CGpuResource& ReadbackBuffer, CPixelBuffer& SrcBuffer);
	void BufferCopy(CGpuResource& Dest, CGpuResource& Src, size_t NumBytes, size_t Offset = 0);

	// Test to see if a fence has already been reached
	bool IsFenceComplete(uint64_t FenceValue);
	// The CPU will wait for a fence to reach a specified value
	void WaitForFence(uint64_t FenceValue);
	// The CPU will wait for all command queues to empty (so that the GPU is idle)
	void IdleGPU(void);

	CCommandQueue& GetCommandQueue(ECommandQueueType Type = kDirectCommandQueue)
	{
		return m_CommandQueueManager.GetCommandQueue(Type);
	}

	CCommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE Type)
	{
		return m_CommandQueueManager.GetCommandQueue(Type);
	}

private:
	bool m_IsInitialized;
	ID3D12Device* m_pDevice;
	CCommandListManager m_CommandListManager;
	CCommandQueueManager m_CommandQueueManager;
};

