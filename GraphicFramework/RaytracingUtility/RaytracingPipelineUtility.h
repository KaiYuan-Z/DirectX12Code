#pragma once
#include "RaytracingBase.h"
#include "RaytracingShaderTable.h"
#include "RayTracingPipelineStateObject.h"
#include "RaytracingDispatchRaysDescriptor.h"
#include "../Core/Model.h"
#include "../Core/CommandList.h"


namespace RaytracingUtility
{
	bool IsDxrSupported(IDXGIAdapter1* pAdapter);

	bool IsDxrSupported(ID3D12Device* pDevice);

	void QueryRaytracingDevice(_In_opt_ ID3D12Device* pDevice, _Inout_opt_ CComPtr<ID3D12Device5>& DxrDevice);

	void QueryRaytracingCommandList(_In_opt_ CCommandList* pCommandList, _Inout_opt_ CComPtr<ID3D12GraphicsCommandList4>& RaytracingCommandList);

	D3D12_DISPATCH_RAYS_DESC MakeDispatchRaysDesc(UINT Widht, UINT Height, const CRaytracingShaderTable* pRayGenerationShaderRecord, const CRaytracingShaderTable* pMissShaderTable = nullptr, const CRaytracingShaderTable* pHitGroupTable = nullptr, const CRaytracingShaderTable* pCallableShaderTable = nullptr);

	void PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc);

	void DispatchRays(_In_ CRayTracingPipelineStateObject* pRaytracingPipelineState, _In_  const D3D12_DISPATCH_RAYS_DESC* pDesc, _In_opt_ CComputeCommandList* pComputeCommandList);

	void DispatchRays(_In_ CRayTracingPipelineStateObject* pRaytracingPipelineState, _In_  CRaytracingDispatchRaysDescriptor* pDesc, _In_opt_ CComputeCommandList* pComputeCommandList);
}
