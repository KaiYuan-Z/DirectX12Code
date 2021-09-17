#pragma once
#include "RaytracingBase.h"
#include "../Core/Model.h"
#include "../Core/ByteAddressBuffer.h"
#include "../Core/UserDescriptorHeap.h"



namespace RaytracingUtility
{	
	struct SGeometryInfo
	{
		UINT VertexCount;
		DXGI_FORMAT VertexFormat;
		UINT VertexBufferStrideInBytes;
		D3D12_GPU_VIRTUAL_ADDRESS VertexBufferGPUAddr;
		UINT IndexCount;
		DXGI_FORMAT IndexFormat;
		D3D12_GPU_VIRTUAL_ADDRESS IndexBufferGPUAddr;
	};

	class CBottomLevelAccelerationStructure
	{
	public:

		CBottomLevelAccelerationStructure();
		~CBottomLevelAccelerationStructure();

		void Create(const std::vector<SGeometryInfo>& GeometryInfoSet, bool EnableUpdate = false);
		void Create(const CModel& Model, bool UsingModelDepthData = false, bool EnableUpdate = false);

		void UpdateAccelerationStructure();

		D3D12_GPU_VIRTUAL_ADDRESS GetAccelerationStructureGpuVirtualAddr() const { return m_AccelerationStructureBuffer.GetGpuVirtualAddress(); }

	private:
		void __BuildAccelerationStructure();

		void __GenGeometryDesc(const CModel& Model, bool UsingModelDepthData, std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& GeometryDescSet);
		void __GenGeometryDesc(const std::vector<SGeometryInfo>& GeometryInfoSet, std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& GeometryDescSet);
		void __GenAccelerationStructure(const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& GeometryDescSet, bool EnableUpdate = false);

		CByteAddressBuffer m_AccelerationStructureBuffer;
		CByteAddressBuffer m_ScratchBuffer;

		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_GeometryDescSet;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_AccelerationStructureDesc;
	};
}

