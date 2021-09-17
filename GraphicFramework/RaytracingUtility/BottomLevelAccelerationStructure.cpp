#include "BottomLevelAccelerationStructure.h"
#include "../Core/GraphicsCore.h"
#include "../Core/CommandList.h"
#include "RaytracingPipelineUtility.h"


using namespace RaytracingUtility;

CBottomLevelAccelerationStructure::CBottomLevelAccelerationStructure()
{
}

CBottomLevelAccelerationStructure::~CBottomLevelAccelerationStructure()
{
}

void CBottomLevelAccelerationStructure::Create(const std::vector<SGeometryInfo>& GeometryInfoSet, bool EnableUpdate)
{
	__GenGeometryDesc(GeometryInfoSet, m_GeometryDescSet);
	__GenAccelerationStructure(m_GeometryDescSet, EnableUpdate);
}

void CBottomLevelAccelerationStructure::Create(const CModel& Model, bool UsingModelDepthData, bool EnableUpdate)
{
	__GenGeometryDesc(Model, UsingModelDepthData, m_GeometryDescSet);
	__GenAccelerationStructure(m_GeometryDescSet, EnableUpdate);
}

void RaytracingUtility::CBottomLevelAccelerationStructure::UpdateAccelerationStructure()
{
	m_AccelerationStructureDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	m_AccelerationStructureDesc.SourceAccelerationStructureData = m_AccelerationStructureBuffer.GetGpuVirtualAddress();
	__BuildAccelerationStructure();
}

void RaytracingUtility::CBottomLevelAccelerationStructure::__BuildAccelerationStructure()
{
	CCommandList* pCommandList = GraphicsCore::BeginCommandList(L"Create Bottom-Level Acceleration Structure");
	_ASSERTE(pCommandList);
	ID3D12GraphicsCommandList* pD3D12CommandList = pCommandList->GetD3D12CommandList();
	_ASSERTE(pD3D12CommandList);
	CComPtr<ID3D12GraphicsCommandList4> pRaytracingCommandList;
	THROW_IF_FAILED(pD3D12CommandList->QueryInterface(IID_PPV_ARGS(&pRaytracingCommandList)));
	pRaytracingCommandList->BuildRaytracingAccelerationStructure(&m_AccelerationStructureDesc, 0, nullptr);
	pCommandList->InsertUAVBarrier(m_AccelerationStructureBuffer, true);
	GraphicsCore::FinishCommandList(pCommandList, false);
}

void CBottomLevelAccelerationStructure::__GenGeometryDesc(const CModel& Model, bool UsingModelDepthData, std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& GeometryDescSet)
{
	UINT NumMeshes = Model.GetHeader().meshCount;
	GeometryDescSet.resize(NumMeshes);
	for (UINT i = 0; i < NumMeshes; i++)
	{
		const CModel::SMesh& Mesh = Model.GetMesh(i);
		GeometryDescSet[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
		GeometryDescSet[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		GeometryDescSet[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		GeometryDescSet[i].Triangles.IndexFormat = (Model.GetIndexDataTypeSize() == sizeof(uint16_t)) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		GeometryDescSet[i].Triangles.Transform3x4 = 0;
		if (!UsingModelDepthData)
		{
			GeometryDescSet[i].Triangles.VertexCount = Mesh.vertexCount;
			GeometryDescSet[i].Triangles.VertexBuffer.StrideInBytes = Mesh.vertexStride;
			GeometryDescSet[i].Triangles.VertexBuffer.StartAddress = Model.GetVertexBuffer().GetGpuVirtualAddress() + (Mesh.vertexDataByteOffset + Mesh.attrib[CModel::attrib_position].offset);
			GeometryDescSet[i].Triangles.IndexCount = Mesh.indexCount;
			GeometryDescSet[i].Triangles.IndexBuffer = Model.GetIndexBuffer().GetGpuVirtualAddress() + Mesh.indexDataByteOffset;
		}
		else
		{
			GeometryDescSet[i].Triangles.VertexCount = Mesh.vertexCountDepth;
			GeometryDescSet[i].Triangles.VertexBuffer.StrideInBytes = Mesh.vertexStrideDepth;
			GeometryDescSet[i].Triangles.VertexBuffer.StartAddress = Model.GetVertexBufferDepth().GetGpuVirtualAddress() + Mesh.vertexDataByteOffsetDepth;
			GeometryDescSet[i].Triangles.IndexCount = Mesh.indexCount;
			GeometryDescSet[i].Triangles.IndexBuffer = Model.GetIndexBufferDepth().GetGpuVirtualAddress() + Mesh.indexDataByteOffset;
		}
	}
}

void CBottomLevelAccelerationStructure::__GenGeometryDesc(const std::vector<SGeometryInfo>& GeometryInfoSet, std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& GeometryDescSet)
{
	GeometryDescSet.resize(GeometryInfoSet.size());

	for (int i = 0; i < GeometryInfoSet.size(); i++)
	{
		GeometryDescSet[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
		GeometryDescSet[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		GeometryDescSet[i].Triangles.VertexCount = GeometryInfoSet[i].VertexCount;
		GeometryDescSet[i].Triangles.VertexFormat = GeometryInfoSet[i].VertexFormat;
		GeometryDescSet[i].Triangles.VertexBuffer.StrideInBytes = GeometryInfoSet[i].VertexBufferStrideInBytes;
		GeometryDescSet[i].Triangles.VertexBuffer.StartAddress = GeometryInfoSet[i].VertexBufferGPUAddr;
		GeometryDescSet[i].Triangles.IndexCount = GeometryInfoSet[i].IndexCount;
		GeometryDescSet[i].Triangles.IndexFormat = GeometryInfoSet[i].IndexFormat;
		GeometryDescSet[i].Triangles.IndexBuffer = GeometryInfoSet[i].IndexBufferGPUAddr;
		GeometryDescSet[i].Triangles.Transform3x4 = 0;
	}
}

void CBottomLevelAccelerationStructure::__GenAccelerationStructure(const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& GeometryDescSet, bool EnableUpdate)
{
	ID3D12Device* pDevice = GraphicsCore::GetD3DDevice();
	_ASSERTE(pDevice);
	CComPtr<ID3D12Device5> DxrDevice;
	RaytracingUtility::QueryRaytracingDevice(pDevice, DxrDevice);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& PrebuildInputInfo = m_AccelerationStructureDesc.Inputs;
	PrebuildInputInfo.Flags = EnableUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	PrebuildInputInfo.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	PrebuildInputInfo.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	PrebuildInputInfo.NumDescs = (UINT)GeometryDescSet.size();
	PrebuildInputInfo.pGeometryDescs = GeometryDescSet.data();
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PrebuildInfo;
	DxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&PrebuildInputInfo, &PrebuildInfo);

	m_AccelerationStructureBuffer.Create(L"Bottom-Level Acceleration Structure", 1, (UINT)PrebuildInfo.ResultDataMaxSizeInBytes, nullptr, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	m_ScratchBuffer.Create(L"Acceleration Structure Scratch Buffer", 1, (UINT)PrebuildInfo.ScratchDataSizeInBytes);

	m_AccelerationStructureDesc.DestAccelerationStructureData = m_AccelerationStructureBuffer.GetGpuVirtualAddress();
	m_AccelerationStructureDesc.ScratchAccelerationStructureData = m_ScratchBuffer.GetGpuVirtualAddress();
	m_AccelerationStructureDesc.SourceAccelerationStructureData = 0;
	
	__BuildAccelerationStructure();
}