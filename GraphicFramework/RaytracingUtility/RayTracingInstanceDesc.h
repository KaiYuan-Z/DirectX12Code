#pragma once
#include "RaytracingBase.h"

namespace RaytracingUtility
{
	struct SRayTracingInstanceDesc : D3D12_RAYTRACING_INSTANCE_DESC
	{
		SRayTracingInstanceDesc()
		{
			ZeroMemory(Transform, sizeof(Transform));
			Transform[0][0] = 1.0f;
			Transform[1][1] = 1.0f;
			Transform[2][2] = 1.0f;
			InstanceID = 0;
			InstanceMask = 0xff;
			InstanceContributionToHitGroupIndex = 0;
			Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			AccelerationStructure = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
		}

		SRayTracingInstanceDesc(D3D12_GPU_VIRTUAL_ADDRESS AccelerationStructureGpuVirtualAddr)
		{
			ZeroMemory(Transform, sizeof(Transform));
			Transform[0][0] = 1.0f;
			Transform[1][1] = 1.0f;
			Transform[2][2] = 1.0f;
			InstanceID = 0;
			InstanceMask = 0xff;
			InstanceContributionToHitGroupIndex = 0;
			Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			AccelerationStructure = AccelerationStructureGpuVirtualAddr;
		}

		void SetTransformMatrix(const DirectX::XMMATRIX& Mat)
		{
			DirectX::XMMATRIX TransposeMat = DirectX::XMMatrixTranspose(Mat);
			memcpy(Transform, &TransposeMat, sizeof(Transform));
		}
	};
}

