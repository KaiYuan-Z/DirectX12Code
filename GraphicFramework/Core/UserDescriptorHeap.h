#pragma once
#include "GraphicsCoreBase.h"
#include "DescriptorHandle.h"


class CUserDescriptorHeap
{
public:

    CUserDescriptorHeap() : m_pDevice(nullptr), m_DescriptorSize(0), m_NumFreeDescriptors(0)
    {
    }

    void Create( ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxCount, const std::wstring& DebugHeapName );

	CDescriptorHandle RequestFreeHandle( uint32_t Count = 1 );
	void AddHandleToHeap( D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart, uint32_t Count = 1 );
	void ResetHandleInHeap( uint32_t Offset, D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptor );

    bool HasAvailableSpace( uint32_t Count ) const { return Count <= m_NumFreeDescriptors; }
	bool ValidateHandle(const CDescriptorHandle& DHandle) const;

	CDescriptorHandle GetHandleAtOffset( uint32_t Offset ) const { _ASSERTE(Offset < GetUsedHandleCount()); return m_FirstHandle + Offset*m_DescriptorSize; }
	uint32_t GetUsedHandleCount() const { return m_HeapDesc.NumDescriptors - m_NumFreeDescriptors; }
	uint32_t GetFreeHandleCount() const { return m_NumFreeDescriptors; }
	uint32_t GetHeapSize() const { return m_HeapDesc.NumDescriptors; }
    ID3D12DescriptorHeap* GetHeapPointer() const { return m_Heap.Get(); }

private:

	ID3D12Device* m_pDevice;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
    D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
    uint32_t m_DescriptorSize;
    uint32_t m_NumFreeDescriptors;
    CDescriptorHandle m_FirstHandle;
    CDescriptorHandle m_NextFreeHandle;
};
