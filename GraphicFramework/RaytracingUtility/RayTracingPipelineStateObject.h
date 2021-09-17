#pragma once
#include "RaytracingBase.h"
#include "../Core/RootSignature.h"

namespace RaytracingUtility
{
	struct SHitGroupDesc
	{
		D3D12_HIT_GROUP_TYPE HitGroupType = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		std::wstring HitGroupName = L"";
		std::wstring AnyHitShaderName = L"";
		std::wstring ClosestHitShaderName = L"";
		std::wstring IntersectionShaderName = L"";
	};

	struct SShaderConfigDesc
	{
		UINT PayloadSize = 0;
		UINT AttributeSize = 0;
	};
	
	class CRayTracingPipelineStateObject
	{
	public:
		CRayTracingPipelineStateObject();
		~CRayTracingPipelineStateObject();
		CRayTracingPipelineStateObject(const CRayTracingPipelineStateObject&) = delete;

		void ResetStateObject();

		void SetShaderProgram(UINT ProgramSize, const void* pProgramData, const std::vector<std::wstring>& ExportNameSet);
		void SetHitGroup(const SHitGroupDesc& HitGroupDesc);
		void SetShaderConfig(const SShaderConfigDesc& ShaderConfigDesc, const std::vector<std::wstring>& AssociatedShaderNameSet);
		void SetGlobalRootSignature(const CRootSignature& BindMappings);
		void SetLocalRootSignature(const CRootSignature& BindMappings, const std::vector<std::wstring>& AssociatedShaderNameSet);
		void SetMaxRecursionDepth(UINT MaxRecursionDepth);

		void FinalizeStateObject();

		void* QueryShaderIdentifier(const std::wstring& ShaderName);

		ID3D12StateObjectPrototype* GetD3D12RayTracingStateObject() const { return m_RayTracingStateObject; }

	private:
		CD3D12_STATE_OBJECT_DESC* m_pRaytracingStateObjectDesc;
		CComPtr<ID3D12StateObjectPrototype> m_RayTracingStateObject;
	};
}

