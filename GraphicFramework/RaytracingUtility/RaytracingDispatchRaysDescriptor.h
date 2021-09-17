#pragma once
#include "RaytracingBase.h"

class CComputeCommandList;

namespace RaytracingUtility
{
	class CRaytracingShaderTable;
	class CRayTracingPipelineStateObject;

	class CRaytracingDispatchRaysDescriptor
	{
	public:
		CRaytracingDispatchRaysDescriptor();
		~CRaytracingDispatchRaysDescriptor();

		void InitDispatchRaysDesc(UINT Widht, UINT Height, const CRaytracingShaderTable* pRayGenerationShaderRecord, const CRaytracingShaderTable* pMissShaderTable = nullptr, const CRaytracingShaderTable* pHitGroupTable = nullptr, const CRaytracingShaderTable* pCallableShaderTable = nullptr);

	private:
		void __ReleaseResources();

	private:
		D3D12_DISPATCH_RAYS_DESC* m_pDispatchRaysDesc = nullptr;
		CRaytracingShaderTable* m_pRayGenShaderTable = nullptr;
		CRaytracingShaderTable* m_pMissShaderTable = nullptr;
		CRaytracingShaderTable* m_pHitGroupTable = nullptr;
		CRaytracingShaderTable* m_pCallableShaderTable = nullptr;

		friend void DispatchRays(_In_ CRayTracingPipelineStateObject* pRaytracingPipelineState, _In_  CRaytracingDispatchRaysDescriptor* pDesc, _In_opt_ CComputeCommandList* pComputeCommandList);
	};
}

