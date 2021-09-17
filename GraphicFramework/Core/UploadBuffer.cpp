#include "UploadBuffer.h"
#include "GraphicsCore.h"


void CUploadBuffer::Create(const std::wstring& Name, uint32_t NumElements, uint32_t ElementSize)
{
	CGpuResource::Destroy();

	m_ElementCount = NumElements;
	m_ElementSize = ElementSize;
	m_BufferSize = NumElements * ElementSize;
	m_UsageState = D3D12_RESOURCE_STATE_GENERIC_READ;

	// Create a readback buffer large enough to hold all texel data
	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	// Readback buffers must be 1-dimensional, i.e. "buffer" not "texture2d"
	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Width = m_BufferSize;
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ASSERT_SUCCEEDED(GraphicsCore::GetD3DDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, MY_IID_PPV_ARGS(&m_pResource)));

	m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

	ReSetResourceName(Name.c_str());

	_CreateDerivedViews();
}

void* CUploadBuffer::Map(void)
{
	THROW_MSG_IF_FALSE(m_pMappedData == nullptr, L"Buffer is already locked.");
	m_pResource->Map(0, &CD3DX12_RANGE(0, m_BufferSize), &m_pMappedData);
	return m_pMappedData;
}

void CUploadBuffer::Unmap(void)
{
	THROW_MSG_IF_FALSE(m_pMappedData != nullptr, L"Buffer is not locked.");
	m_pResource->Unmap(0, &CD3DX12_RANGE(0, 0));
	m_pMappedData = nullptr;
}

void CUploadBuffer::_CreateDerivedViews(void)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Buffer.NumElements = m_ElementCount;
	SRVDesc.Buffer.StructureByteStride = m_ElementSize;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_SRV = GraphicsCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	GraphicsCore::GetD3DDevice()->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV);
}
