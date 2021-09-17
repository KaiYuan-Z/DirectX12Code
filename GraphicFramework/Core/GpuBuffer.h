#pragma once
#include "GpuResource.h"

class CCommandList;

class CGpuBuffer : public CGpuResource
{
public:
    virtual ~CGpuBuffer() { Destroy(); }

    // Create a buffer.  If initial data is provided, it will be copied into the buffer using the default command list.
    void Create( const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, 
		const void* initialData = nullptr, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON);

    // Sub-__Allocate a buffer out of a pre-allocated heap.  If initial data is provided, it will be copied into the buffer using the default command list.
    void CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize,
		const void* initialData = nullptr, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON);

    const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(void) const { return m_UAV; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const { return m_SRV; }

    //D3D12_GPU_VIRTUAL_ADDRESS GetRootConstantBufferView(void) const { return m_GpuVirtualAddress; }

    D3D12_CPU_DESCRIPTOR_HANDLE CreateConstantBufferView( uint32_t Offset, uint32_t Size ) const;

    D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const;
    D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(size_t BaseVertexIndex = 0) const
    {
        size_t Offset = BaseVertexIndex * m_ElementSize;
        return CreateVertexBufferView(Offset, (uint32_t)(m_BufferSize - Offset), m_ElementSize);
    }

    D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView(size_t Offset, uint32_t Size, bool b32Bit = false) const;
    D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView(size_t StartIndex = 0) const
    {
        size_t Offset = StartIndex * m_ElementSize;
        return CreateIndexBufferView(Offset, (uint32_t)(m_BufferSize - Offset), m_ElementSize == 4);
    }

    size_t GetBufferSize() const { return m_BufferSize; }
    uint32_t GetElementCount() const { return m_ElementCount; }
    uint32_t GetElementSize() const { return m_ElementSize; }

protected:
	CGpuBuffer(void) : m_BufferSize(0), m_ElementCount(0), m_ElementSize(0)
	{
		m_ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		m_UAV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_SRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

    D3D12_RESOURCE_DESC _DescribeBuffer(void);
    virtual void _CreateDerivedViews(void) = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE m_UAV;
    D3D12_CPU_DESCRIPTOR_HANDLE m_SRV;

    size_t m_BufferSize;
    uint32_t m_ElementCount;
    uint32_t m_ElementSize;
    D3D12_RESOURCE_FLAGS m_ResourceFlags;
};

inline D3D12_VERTEX_BUFFER_VIEW CGpuBuffer::CreateVertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const
{
    D3D12_VERTEX_BUFFER_VIEW VBView;
    VBView.BufferLocation = m_GpuVirtualAddress + Offset;
    VBView.SizeInBytes = Size;
    VBView.StrideInBytes = Stride;
    return VBView;
}

inline D3D12_INDEX_BUFFER_VIEW CGpuBuffer::CreateIndexBufferView(size_t Offset, uint32_t Size, bool b32Bit) const
{
    D3D12_INDEX_BUFFER_VIEW IBView;
    IBView.BufferLocation = m_GpuVirtualAddress + Offset;
    IBView.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    IBView.SizeInBytes = Size;
    return IBView;
}