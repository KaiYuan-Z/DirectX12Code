#pragma once
#include "RaytracingBase.h"
#include "RayTracingInstanceDesc.h"
#include "../Core/ByteAddressBuffer.h"
#include "../Core/UserDescriptorHeap.h"
#include "../Core/UploadBuffer.h"

namespace RaytracingUtility
{
	class CTopLevelAccelerationStructure
	{
	public:

		CTopLevelAccelerationStructure();
		~CTopLevelAccelerationStructure();

		void Create(const std::vector<SRayTracingInstanceDesc>& InstanceDescSet, bool EnableUpdate = false, bool HasAnimations = false);

		void SetInstanceTransform(UINT InstanceIndex, const DirectX::XMMATRIX& Mat);
		void UpdateAccelerationStructure();

		D3D12_GPU_VIRTUAL_ADDRESS GetAccelerationStructureGpuVirtualAddr() const { return m_AccelerationStructureBuffer.GetGpuVirtualAddress(); }
		CByteAddressBuffer& GetAccelerationStructureBufferData() { return m_AccelerationStructureBuffer; }
		
		bool IsUpdatable() const { return m_EnableUpdate; }

	private:

		void __BuildAccelerationStructure();
		void __BuildInstanceDataBuffer();

		D3D12_GPU_VIRTUAL_ADDRESS __GetInstanceDataBufferGpuVirtualAddr() const;

		CByteAddressBuffer m_ScratchBuffer;
		CByteAddressBuffer m_InstanceDataBufferStatic;
		CUploadBuffer m_InstanceDataBufferDynamic;
		CByteAddressBuffer m_AccelerationStructureBuffer;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_TopLevelAccelerationStructureDesc;
		std::vector<SRayTracingInstanceDesc> m_InstanceDescSet;

		void* m_pInstanceDataBufferDynamicMappedData = nullptr;
		uint64_t m_UpdateFence = 0;

		bool m_EnableUpdate = false;
		bool m_HasAnimations = false;
		bool m_IsTransformDataNeedUpdate = false;
	};
}