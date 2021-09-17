#include "UserDescriptorHeap.h"


//
// UserDescriptorHeap implementation
//

void CUserDescriptorHeap::Create( ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxCount, const std::wstring& DebugHeapName )
{
	_ASSERTE(pDevice && MaxCount > 0);
	m_pDevice = pDevice;
	m_HeapDesc.NumDescriptors = MaxCount;
	m_HeapDesc.Type = Type;
	m_HeapDesc.Flags = ((Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) || (Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)) ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	m_HeapDesc.NodeMask = 1;
	ASSERT_SUCCEEDED(m_pDevice->CreateDescriptorHeap(&m_HeapDesc, MY_IID_PPV_ARGS(m_Heap.ReleaseAndGetAddressOf())));

#ifdef RELEASE
    (void)DebugHeapName;
#else
    m_Heap->SetName(DebugHeapName.c_str());
#endif

    m_DescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(m_HeapDesc.Type);
    m_NumFreeDescriptors = m_HeapDesc.NumDescriptors;
    m_FirstHandle = CDescriptorHandle( m_Heap->GetCPUDescriptorHandleForHeapStart(),  m_Heap->GetGPUDescriptorHandleForHeapStart() );
    m_NextFreeHandle = m_FirstHandle;
}

CDescriptorHandle CUserDescriptorHeap::RequestFreeHandle( uint32_t Count )
{
    ASSERT(HasAvailableSpace(Count), "Descriptor Heap out of space.  Increase heap size.");
    CDescriptorHandle ret = m_NextFreeHandle;
    m_NextFreeHandle += Count * m_DescriptorSize;
	m_NumFreeDescriptors -= Count;
    return ret;
}

void CUserDescriptorHeap::AddHandleToHeap( D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart, uint32_t Count )
{
	_ASSERTE(Count > 0);
	CDescriptorHandle DestDescriptorRangeStart = RequestFreeHandle(Count);
	_ASSERTE(m_pDevice);
	m_pDevice->CopyDescriptorsSimple(Count, DestDescriptorRangeStart.GetCpuHandle(), SrcDescriptorRangeStart, m_HeapDesc.Type);
}

void CUserDescriptorHeap::ResetHandleInHeap( uint32_t Offset, D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor )
{
	CDescriptorHandle DestDescriptor = GetHandleAtOffset(Offset);
	_ASSERTE(m_pDevice);
	m_pDevice->CopyDescriptorsSimple(1, DestDescriptor.GetCpuHandle(), SrcDescriptor, m_HeapDesc.Type);
}

bool CUserDescriptorHeap::ValidateHandle( const CDescriptorHandle& DHandle ) const
{
    if (DHandle.GetCpuHandle().ptr < m_FirstHandle.GetCpuHandle().ptr ||
        DHandle.GetCpuHandle().ptr >= m_FirstHandle.GetCpuHandle().ptr + m_HeapDesc.NumDescriptors * m_DescriptorSize)
        return false;

    if (DHandle.GetGpuHandle().ptr - m_FirstHandle.GetGpuHandle().ptr !=
        DHandle.GetCpuHandle().ptr - m_FirstHandle.GetCpuHandle().ptr)
        return false;

    return true;
}
