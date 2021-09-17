#include "StructuredBuffer.h"
#include "GraphicsCore.h"

void CStructuredBuffer::_CreateDerivedViews(void)
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

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	UAVDesc.Buffer.CounterOffsetInBytes = 0;
	UAVDesc.Buffer.NumElements = m_ElementCount;
	UAVDesc.Buffer.StructureByteStride = m_ElementSize;
	UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	m_CounterBuffer.Create(L"StructuredBuffer::Counter", 1, 4);

	if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_UAV = GraphicsCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	GraphicsCore::GetD3DDevice()->CreateUnorderedAccessView(m_pResource.Get(), m_CounterBuffer.GetResource(), &UAVDesc, m_UAV);
}

const D3D12_CPU_DESCRIPTOR_HANDLE& CStructuredBuffer::GetCounterSRV(CCommandList& CommandList)
{
	CommandList.TransitionResource(m_CounterBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
	return m_CounterBuffer.GetSRV();
}

const D3D12_CPU_DESCRIPTOR_HANDLE& CStructuredBuffer::GetCounterUAV(CCommandList& CommandList)
{
	CommandList.TransitionResource(m_CounterBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	return m_CounterBuffer.GetUAV();
}