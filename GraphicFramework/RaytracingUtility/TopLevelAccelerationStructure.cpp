#include "TopLevelAccelerationStructure.h"
#include "../Core/GraphicsCore.h"
#include "../Core/CommandList.h"
#include "RaytracingPipelineUtility.h"


using namespace RaytracingUtility;

CTopLevelAccelerationStructure::CTopLevelAccelerationStructure()
{
}

CTopLevelAccelerationStructure::~CTopLevelAccelerationStructure()
{
}

void CTopLevelAccelerationStructure::Create(const std::vector<SRayTracingInstanceDesc>& InstanceDescSet, bool EnableUpdate, bool HasAnimations)
{
	ID3D12Device* pDevice = GraphicsCore::GetD3DDevice();
	_ASSERTE(pDevice);
	CComPtr<ID3D12Device5> DxrDevice;
	RaytracingUtility::QueryRaytracingDevice(pDevice, DxrDevice);
	
	m_InstanceDescSet = InstanceDescSet;
	m_EnableUpdate = EnableUpdate;
	m_HasAnimations = HasAnimations;

	__BuildInstanceDataBuffer();
	
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& PrebuildInputInfo = m_TopLevelAccelerationStructureDesc.Inputs;
	PrebuildInputInfo.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	PrebuildInputInfo.Flags = m_EnableUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	PrebuildInputInfo.NumDescs = (UINT)m_InstanceDescSet.size();
	PrebuildInputInfo.pGeometryDescs = nullptr;
	PrebuildInputInfo.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	PrebuildInputInfo.InstanceDescs = __GetInstanceDataBufferGpuVirtualAddr();
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PrebuildInfo;
	DxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&PrebuildInputInfo, &PrebuildInfo);

	m_AccelerationStructureBuffer.Create(L"TopLevelAccelerationStructure", 1, (UINT)PrebuildInfo.ResultDataMaxSizeInBytes, nullptr, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	m_ScratchBuffer.Create(L"Acceleration Structure Scratch Buffer", 1, (UINT)PrebuildInfo.ScratchDataSizeInBytes);

	m_TopLevelAccelerationStructureDesc.DestAccelerationStructureData = m_AccelerationStructureBuffer->GetGPUVirtualAddress();
	m_TopLevelAccelerationStructureDesc.ScratchAccelerationStructureData = m_ScratchBuffer.GetGpuVirtualAddress();
	m_TopLevelAccelerationStructureDesc.SourceAccelerationStructureData = 0;

	__BuildAccelerationStructure();
}

void CTopLevelAccelerationStructure::SetInstanceTransform(UINT InstanceIndex, const DirectX::XMMATRIX& Mat)
{
	_ASSERTE(InstanceIndex < m_InstanceDescSet.size());
	m_InstanceDescSet[InstanceIndex].SetTransformMatrix(Mat);
	m_IsTransformDataNeedUpdate = true;
}

void CTopLevelAccelerationStructure::UpdateAccelerationStructure()
{
	if (!m_HasAnimations && !m_IsTransformDataNeedUpdate) return;
	
	_ASSERTE(m_EnableUpdate && m_pInstanceDataBufferDynamicMappedData);

	CCommandQueue& commandQueue = GraphicsCore::GetCommandQueue();
	if (m_IsTransformDataNeedUpdate)
	{
		commandQueue.WaitForFence(m_UpdateFence);
		memcpy(m_pInstanceDataBufferDynamicMappedData, m_InstanceDescSet.data(), (UINT)m_InstanceDataBufferDynamic.GetBufferSize());
	}	
	
	m_TopLevelAccelerationStructureDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	m_TopLevelAccelerationStructureDesc.SourceAccelerationStructureData = m_AccelerationStructureBuffer.GetGpuVirtualAddress();
	__BuildAccelerationStructure();

	if (m_IsTransformDataNeedUpdate)
	{
		m_UpdateFence = commandQueue.IncrementFence();
	}

	m_IsTransformDataNeedUpdate = false;
}

void CTopLevelAccelerationStructure::__BuildAccelerationStructure()
{
	CCommandList* pCommandList = GraphicsCore::BeginCommandList(L"Create Top-Level Acceleration Structure");
	_ASSERTE(pCommandList);
	ID3D12GraphicsCommandList* pD3D12CommandList = pCommandList->GetD3D12CommandList();
	_ASSERTE(pD3D12CommandList);
	CComPtr<ID3D12GraphicsCommandList4> pRaytracingCommandList;
	THROW_IF_FAILED(pD3D12CommandList->QueryInterface(IID_PPV_ARGS(&pRaytracingCommandList)));
	pRaytracingCommandList->BuildRaytracingAccelerationStructure(&m_TopLevelAccelerationStructureDesc, 0, nullptr);
	pCommandList->InsertUAVBarrier(m_AccelerationStructureBuffer, true);
	GraphicsCore::FinishCommandList(pCommandList, false);
}

void CTopLevelAccelerationStructure::__BuildInstanceDataBuffer()
{
	if (m_EnableUpdate)
	{
		m_InstanceDataBufferDynamic.Create(L"Instance Data Buffer", (UINT)m_InstanceDescSet.size(), sizeof(SRayTracingInstanceDesc));
		m_pInstanceDataBufferDynamicMappedData = m_InstanceDataBufferDynamic.Map(); 
		memcpy(m_pInstanceDataBufferDynamicMappedData, m_InstanceDescSet.data(), (UINT)m_InstanceDataBufferDynamic.GetBufferSize());
	}
	else
	{
		m_InstanceDataBufferStatic.Create(L"Instance Data Buffer", (UINT)m_InstanceDescSet.size(), (UINT)sizeof(SRayTracingInstanceDesc), m_InstanceDescSet.data());
	}
}

D3D12_GPU_VIRTUAL_ADDRESS CTopLevelAccelerationStructure::__GetInstanceDataBufferGpuVirtualAddr() const
{
	if (m_EnableUpdate)
		return m_InstanceDataBufferDynamic.GetGpuVirtualAddress();
	else
		return m_InstanceDataBufferStatic.GetGpuVirtualAddress();
}
