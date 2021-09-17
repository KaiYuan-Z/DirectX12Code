#include "GpuBuffer.h"
#include "GraphicsCore.h"

void CGpuBuffer::Create( const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, 
	const void* initialData, D3D12_RESOURCE_STATES initialResourceState)
{
    CGpuResource::Destroy();

    m_ElementCount = NumElements;
    m_ElementSize = ElementSize;
    m_BufferSize = NumElements * ElementSize;

    D3D12_RESOURCE_DESC ResourceDesc = _DescribeBuffer();

    m_UsageState = initialResourceState;

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    ASSERT_SUCCEEDED( 
		GraphicsCore::GetD3DDevice()->CreateCommittedResource( &HeapProps, D3D12_HEAP_FLAG_NONE,
        &ResourceDesc, m_UsageState, nullptr, MY_IID_PPV_ARGS(&m_pResource)) );

    m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

	if (initialData)
		GraphicsCore::InitializeBuffer(*this, initialData, m_BufferSize);

	ReSetResourceName(name.c_str());

    _CreateDerivedViews();
}

// Sub-__Allocate a buffer out of a pre-allocated heap.  If initial data is provided, it will be copied into the buffer using the default command list.
void CGpuBuffer::CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize,
	const void* initialData, D3D12_RESOURCE_STATES initialResourceState)
{
    m_ElementCount = NumElements;
    m_ElementSize = ElementSize;
    m_BufferSize = NumElements * ElementSize;

    D3D12_RESOURCE_DESC ResourceDesc = _DescribeBuffer();

    m_UsageState = initialResourceState;

    ASSERT_SUCCEEDED(GraphicsCore::GetD3DDevice()->CreatePlacedResource(pBackingHeap, HeapOffset, &ResourceDesc, m_UsageState, nullptr, MY_IID_PPV_ARGS(&m_pResource)));

    m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

    if (initialData)
		GraphicsCore::InitializeBuffer(*this, initialData, m_BufferSize);

	ReSetResourceName(name.c_str());

    _CreateDerivedViews();
}

D3D12_CPU_DESCRIPTOR_HANDLE CGpuBuffer::CreateConstantBufferView(uint32_t Offset, uint32_t Size) const
{
    ASSERT(Offset + Size <= m_BufferSize);

    Size = MathUtility::AlignUp(Size, 16);

    D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
    CBVDesc.BufferLocation = m_GpuVirtualAddress + (size_t)Offset;
    CBVDesc.SizeInBytes = Size;

    D3D12_CPU_DESCRIPTOR_HANDLE hCBV = GraphicsCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	GraphicsCore::GetD3DDevice()->CreateConstantBufferView(&CBVDesc, hCBV);
    return hCBV;
}

D3D12_RESOURCE_DESC CGpuBuffer::_DescribeBuffer(void)
{
    ASSERT(m_BufferSize != 0);

    D3D12_RESOURCE_DESC Desc = {};
    Desc.Alignment = 0;
    Desc.DepthOrArraySize = 1;
    Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags = m_ResourceFlags;
    Desc.Format = DXGI_FORMAT_UNKNOWN;
    Desc.Height = 1;
    Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.MipLevels = 1;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.Width = (UINT64)m_BufferSize;
    return Desc;
}
