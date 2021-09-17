#include "CommandManager.h"


CCommandManager::CCommandManager() : m_IsInitialized(false), m_pDevice(nullptr)
{
}

CCommandManager::~CCommandManager()
{
	Shutdown();
}

void CCommandManager::Initialize(ID3D12Device* pDevice)
{
	if (!m_IsInitialized)
	{
		_ASSERTE(pDevice);
		m_pDevice = pDevice;
		m_CommandQueueManager.Initialize(pDevice);
		m_CommandListManager.Initialize(pDevice, &m_CommandQueueManager);
		m_IsInitialized = true;
	}
}

void CCommandManager::Shutdown()
{
	if (m_IsInitialized)
	{
		m_CommandListManager.Shutdown();
		m_CommandQueueManager.Shutdown();
		m_IsInitialized = false;
	}
}

CCommandList* CCommandManager::BeginCommandList(const std::wstring& ID)
{
	_ASSERTE(m_IsInitialized);	
	CCommandList* pNewCommandList = m_CommandListManager.AllocateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	_ASSERTE(pNewCommandList);
	pNewCommandList->_SetID(ID);
	pNewCommandList->_SetCategory(kDirectCommandList);
	return pNewCommandList;
}

CGraphicsCommandList* CCommandManager::BeginGraphicsCommandList(const std::wstring& ID)
{
	_ASSERTE(m_IsInitialized);
	CCommandList* pNewCommandList = m_CommandListManager.AllocateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	_ASSERTE(pNewCommandList);
	pNewCommandList->_SetID(ID);
	pNewCommandList->_SetCategory(kGraphicsCommandList);
	return static_cast<CGraphicsCommandList*>(pNewCommandList);
}

CComputeCommandList* CCommandManager::BeginComputeCommandList(const std::wstring& ID, bool Async)
{
	_ASSERTE(m_IsInitialized);
	CCommandList* pNewCommandList = m_CommandListManager.AllocateCommandList(Async ? D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT);
	_ASSERTE(pNewCommandList);
	pNewCommandList->_SetID(ID);
	pNewCommandList->_SetCategory(kComputeCommandList);
	return static_cast<CComputeCommandList*>(pNewCommandList);
}

uint64_t CCommandManager::FinishCommandList(CCommandList*& pCommandList, bool WaitForCompletion)
{
	_ASSERTE(m_IsInitialized && pCommandList);
	ECommandListCategory CommandListOriginalCategory = pCommandList->_GetOriginalCategory();
	_ASSERTE(CommandListOriginalCategory == kDirectCommandList); // Checking the command list's original category, which prevents raw command list from being terminated twice.
	uint64_t FenceValue = pCommandList->__Finish();
	m_CommandListManager.FreeCommandList(pCommandList);
	pCommandList = nullptr;
	if(WaitForCompletion) WaitForFence(FenceValue);
	return FenceValue;
}

uint64_t CCommandManager::FinishCommandList(CGraphicsCommandList*& pCommandList, bool WaitForCompletion)
{
	_ASSERTE(m_IsInitialized && pCommandList);
	ECommandListCategory CommandListOriginalCategory = pCommandList->_GetOriginalCategory();
	_ASSERTE(CommandListOriginalCategory == kGraphicsCommandList); // Checking the command list's original category, which prevents raw command list from being terminated twice.
	uint64_t FenceValue = pCommandList->__Finish();
	m_CommandListManager.FreeCommandList(pCommandList);
	pCommandList = nullptr;
	if (WaitForCompletion) WaitForFence(FenceValue);
	return FenceValue;
}

uint64_t CCommandManager::FinishCommandList(CComputeCommandList*& pCommandList, bool WaitForCompletion)
{
	_ASSERTE(m_IsInitialized && pCommandList);
	ECommandListCategory CommandListOriginalCategory = pCommandList->_GetOriginalCategory();
	_ASSERTE(CommandListOriginalCategory == kComputeCommandList); // Checking the command list's original category, which prevents raw command list from being terminated twice.
	uint64_t FenceValue = pCommandList->__Finish();
	m_CommandListManager.FreeCommandList(pCommandList);
	pCommandList = nullptr;
	if (WaitForCompletion) WaitForFence(FenceValue);
	return FenceValue;
}

void CCommandManager::InitializeTexture(CGpuResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[])
{
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest.GetResource(), 0, NumSubresources);

	CCommandList* pInitCommandList = BeginCommandList();
	_ASSERTE(pInitCommandList);

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	pInitCommandList->TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
	SDynAlloc mem = pInitCommandList->ReserveUploadMemory(uploadBufferSize);
	UpdateSubresources(pInitCommandList->m_CommandList, Dest.GetResource(), mem.Buffer.GetResource(), 0, 0, NumSubresources, SubData);
	pInitCommandList->TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	FinishCommandList(pInitCommandList, true);
}

void CCommandManager::InitializeBuffer(CGpuResource& Dest, const void* pBufferData, size_t NumBytes, size_t Offset)
{
	CCommandList* pInitCommandList = BeginCommandList();
	_ASSERTE(pInitCommandList);

	SDynAlloc mem = pInitCommandList->ReserveUploadMemory(NumBytes);
	MemCopy(mem.DataPtr, pBufferData, NumBytes);

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	pInitCommandList->TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
	ID3D12GraphicsCommandList* pCommandList = pInitCommandList->m_CommandList;
	_ASSERTE(pCommandList);
	pCommandList->CopyBufferRegion(Dest.GetResource(), Offset, mem.Buffer.GetResource(), 0, NumBytes);
	pInitCommandList->TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	FinishCommandList(pInitCommandList, true);
}

void CCommandManager::InitializeTextureArraySlice(CGpuResource & Dest, UINT SliceIndex, CGpuResource & Src)
{
	CCommandList* pInitCommandList = BeginCommandList();
	_ASSERTE(pInitCommandList);

	pInitCommandList->TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
	pInitCommandList->FlushResourceBarriers();

	const D3D12_RESOURCE_DESC& DestDesc = Dest.GetResource()->GetDesc();
	const D3D12_RESOURCE_DESC& SrcDesc = Src.GetResource()->GetDesc();

	ASSERT(SliceIndex < DestDesc.DepthOrArraySize &&
		SrcDesc.DepthOrArraySize == 1 &&
		DestDesc.Width == SrcDesc.Width &&
		DestDesc.Height == SrcDesc.Height &&
		DestDesc.MipLevels <= SrcDesc.MipLevels
	);

	UINT SubResourceIndex = SliceIndex * DestDesc.MipLevels;

	for (UINT i = 0; i < DestDesc.MipLevels; ++i)
	{
		D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
		{
			Dest.GetResource(),
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			SubResourceIndex + i
		};

		D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
		{
			Src.GetResource(),
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			i
		};

		ID3D12GraphicsCommandList* pCommandList = pInitCommandList->m_CommandList;
		_ASSERTE(pCommandList);
		pCommandList->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
	}

	pInitCommandList->TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);
	
	FinishCommandList(pInitCommandList, true);
}

void CCommandManager::ReadbackTexture2D(CGpuResource & ReadbackBuffer, CPixelBuffer & SrcBuffer)
{
	// The footprint may depend on the device of the resource, but we assume there is only one device.
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;
	m_pDevice->GetCopyableFootprints(&SrcBuffer.GetResource()->GetDesc(), 0, 1, 0, &PlacedFootprint, nullptr, nullptr, nullptr);

	// This very short command list only issues one API call and will be synchronized so we can immediately read
	// the buffer contents.
	CCommandList* pInitCommandList = BeginCommandList();
	_ASSERTE(pInitCommandList);

	pInitCommandList->TransitionResource(SrcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, true);

	pInitCommandList->m_CommandList->CopyTextureRegion(
		&CD3DX12_TEXTURE_COPY_LOCATION(ReadbackBuffer.GetResource(), PlacedFootprint), 0, 0, 0,
		&CD3DX12_TEXTURE_COPY_LOCATION(SrcBuffer.GetResource(), 0), nullptr);

	FinishCommandList(pInitCommandList, true);
}

void CCommandManager::BufferCopy(CGpuResource& Dest, CGpuResource& Src, size_t NumBytes, size_t Offset)
{
	CCommandList* pInitCommandList = BeginCommandList();
	_ASSERTE(pInitCommandList);

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	pInitCommandList->TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE, false);
	pInitCommandList->TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
	ID3D12GraphicsCommandList* pCommandList = pInitCommandList->m_CommandList;
	_ASSERTE(pCommandList);
	pCommandList->CopyBufferRegion(Dest.GetResource(), Offset, Src.GetResource(), 0, NumBytes);
	pInitCommandList->TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	FinishCommandList(pInitCommandList, true);
}

bool CCommandManager::IsFenceComplete(uint64_t FenceValue)
{
	_ASSERTE(m_IsInitialized);
	return m_CommandQueueManager.GetCommandQueue(D3D12_COMMAND_LIST_TYPE(FenceValue >> 56)).IsFenceComplete(FenceValue);
}

void CCommandManager::WaitForFence(uint64_t FenceValue)
{
	_ASSERTE(m_IsInitialized);
	CCommandQueue& Producer = m_CommandQueueManager.GetCommandQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
	Producer.WaitForFence(FenceValue);
}

void CCommandManager::IdleGPU(void)
{
	m_CommandQueueManager.GetCommandQueue(kDirectCommandQueue).WaitForIdle();
	m_CommandQueueManager.GetCommandQueue(kComputeCommandQueue).WaitForIdle();
	m_CommandQueueManager.GetCommandQueue(kCopyCommandQueue).WaitForIdle();
}
