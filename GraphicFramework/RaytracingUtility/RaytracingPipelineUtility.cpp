#include "RaytracingPipelineUtility.h"

using namespace RaytracingUtility;


bool RaytracingUtility::IsDxrSupported(IDXGIAdapter1* pAdapter)
{
	ComPtr<ID3D12Device> testDevice;
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};

	return SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
		&& SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
		&& featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

bool RaytracingUtility::IsDxrSupported(ID3D12Device* pDevice)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
	return SUCCEEDED(pDevice && pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
		&& featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

void RaytracingUtility::QueryRaytracingDevice(ID3D12Device* pDevice, CComPtr<ID3D12Device5>& DxrDevice)
{
	_ASSERTE(pDevice);
	THROW_MSG_IF_FAILED(pDevice->QueryInterface(IID_PPV_ARGS(&DxrDevice)), L"Couldn't get DirectX Raytracing interface for the device.\n");
}

void RaytracingUtility::QueryRaytracingCommandList(CCommandList* pCommandList, CComPtr<ID3D12GraphicsCommandList4>& RaytracingCommandList)
{
	_ASSERTE(pCommandList);
	ID3D12GraphicsCommandList* pD3D12CommandList = pCommandList->GetD3D12CommandList();
	_ASSERTE(pD3D12CommandList);
	THROW_MSG_IF_FAILED(pD3D12CommandList->QueryInterface(IID_PPV_ARGS(&RaytracingCommandList)), L"Couldn't get DirectX Raytracing interface for the command list.\n");
}

D3D12_DISPATCH_RAYS_DESC RaytracingUtility::MakeDispatchRaysDesc(UINT Widht, UINT Height, const CRaytracingShaderTable* pRayGenerationShaderRecord, const CRaytracingShaderTable* pMissShaderTable, const CRaytracingShaderTable* pHitGroupTable, const CRaytracingShaderTable* pCallableShaderTable)
{
	_ASSERTE(Widht > 0 && Height > 0);
	_ASSERTE(pRayGenerationShaderRecord && pRayGenerationShaderRecord->GetShaderRecordCount() == 1);

	D3D12_DISPATCH_RAYS_DESC DispatchRaysDesc;

	DispatchRaysDesc.Width = Widht;
	DispatchRaysDesc.Height = Height;
	DispatchRaysDesc.Depth = 1;

	DispatchRaysDesc.RayGenerationShaderRecord.StartAddress = pRayGenerationShaderRecord->GetShaderTableGpuVirtualAddr();
	DispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = pRayGenerationShaderRecord->GetShaderTableSizeInByte();

	if (pMissShaderTable)
	{
		DispatchRaysDesc.MissShaderTable.StartAddress = pMissShaderTable->GetShaderTableGpuVirtualAddr();
		DispatchRaysDesc.MissShaderTable.SizeInBytes = pMissShaderTable->GetShaderTableSizeInByte();
		DispatchRaysDesc.MissShaderTable.StrideInBytes = pMissShaderTable->GetShaderRecordStrideSizeInByte();
	}
	else
	{
		DispatchRaysDesc.MissShaderTable.StartAddress = 0;
		DispatchRaysDesc.MissShaderTable.SizeInBytes = 0;
		DispatchRaysDesc.MissShaderTable.StrideInBytes = 0;
	}
	if (pHitGroupTable)
	{
		DispatchRaysDesc.HitGroupTable.StartAddress = pHitGroupTable->GetShaderTableGpuVirtualAddr();
		DispatchRaysDesc.HitGroupTable.SizeInBytes = pHitGroupTable->GetShaderTableSizeInByte();
		DispatchRaysDesc.HitGroupTable.StrideInBytes = pHitGroupTable->GetShaderRecordStrideSizeInByte();
	}
	else
	{
		DispatchRaysDesc.HitGroupTable.StartAddress = 0;
		DispatchRaysDesc.HitGroupTable.SizeInBytes = 0;
		DispatchRaysDesc.HitGroupTable.StrideInBytes = 0;
	}
	if (pCallableShaderTable)
	{
		DispatchRaysDesc.CallableShaderTable.StartAddress = pCallableShaderTable->GetShaderTableGpuVirtualAddr();
		DispatchRaysDesc.CallableShaderTable.SizeInBytes = pCallableShaderTable->GetShaderTableSizeInByte();
		DispatchRaysDesc.CallableShaderTable.StrideInBytes = pCallableShaderTable->GetShaderRecordStrideSizeInByte();
	}
	else
	{
		DispatchRaysDesc.CallableShaderTable.StartAddress = 0;
		DispatchRaysDesc.CallableShaderTable.SizeInBytes = 0;
		DispatchRaysDesc.CallableShaderTable.StrideInBytes = 0;
	}

	return DispatchRaysDesc;
}

void RaytracingUtility::PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc)
{
	std::wstringstream wstr;
	wstr << L"\n";
	wstr << L"--------------------------------------------------------------------\n";
	wstr << L"| D3D12 State Object 0x" << static_cast<const void*>(desc) << L": ";
	if (desc->Type == D3D12_STATE_OBJECT_TYPE_COLLECTION) wstr << L"Collection\n";
	if (desc->Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE) wstr << L"Raytracing Pipeline\n";

	auto ExportTree = [](UINT depth, UINT numExports, const D3D12_EXPORT_DESC* exports)
	{
		std::wostringstream woss;
		for (UINT i = 0; i < numExports; i++)
		{
			woss << L"|";
			if (depth > 0)
			{
				for (UINT j = 0; j < 2 * depth - 1; j++) woss << L" ";
			}
			woss << L" [" << i << L"]: ";
			if (exports[i].ExportToRename) woss << exports[i].ExportToRename << L" --> ";
			woss << exports[i].Name << L"\n";
		}
		return woss.str();
	};

	for (UINT i = 0; i < desc->NumSubobjects; i++)
	{
		wstr << L"| [" << i << L"]: ";
		switch (desc->pSubobjects[i].Type)
		{
		case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
			wstr << L"Global Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
			break;
		case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
			wstr << L"Local Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
			break;
		case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
			wstr << L"Node Mask: 0x" << std::hex << std::setfill(L'0') << std::setw(8) << *static_cast<const UINT*>(desc->pSubobjects[i].pDesc) << std::setw(0) << std::dec << L"\n";
			break;
		case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY:
		{
			wstr << L"DXIL Library 0x";
			auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[i].pDesc);
			wstr << lib->DXILLibrary.pShaderBytecode << L", " << lib->DXILLibrary.BytecodeLength << L" bytes\n";
			wstr << ExportTree(1, lib->NumExports, lib->pExports);
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION:
		{
			wstr << L"Existing Library 0x";
			auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(desc->pSubobjects[i].pDesc);
			wstr << collection->pExistingCollection << L"\n";
			wstr << ExportTree(1, collection->NumExports, collection->pExports);
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
		{
			wstr << L"Subobject to Exports Association (Subobject [";
			auto association = static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
			UINT index = static_cast<UINT>(association->pSubobjectToAssociate - desc->pSubobjects);
			wstr << index << L"])\n";
			for (UINT j = 0; j < association->NumExports; j++)
			{
				wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
			}
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
		{
			wstr << L"DXIL Subobjects to Exports Association (";
			auto association = static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
			wstr << association->SubobjectToAssociate << L")\n";
			for (UINT j = 0; j < association->NumExports; j++)
			{
				wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
			}
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG:
		{
			wstr << L"Raytracing Shader Config\n";
			auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(desc->pSubobjects[i].pDesc);
			wstr << L"|  [0]: Max Payload Size: " << config->MaxPayloadSizeInBytes << L" bytes\n";
			wstr << L"|  [1]: Max Attribute Size: " << config->MaxAttributeSizeInBytes << L" bytes\n";
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG:
		{
			wstr << L"Raytracing Pipeline Config\n";
			auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(desc->pSubobjects[i].pDesc);
			wstr << L"|  [0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth << L"\n";
			break;
		}
		case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP:
		{
			wstr << L"Hit Group (";
			auto hitGroup = static_cast<const D3D12_HIT_GROUP_DESC*>(desc->pSubobjects[i].pDesc);
			wstr << (hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]") << L")\n";
			wstr << L"|  [0]: Any Hit Import: " << (hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport : L"[none]") << L"\n";
			wstr << L"|  [1]: Closest Hit Import: " << (hitGroup->ClosestHitShaderImport ? hitGroup->ClosestHitShaderImport : L"[none]") << L"\n";
			wstr << L"|  [2]: Intersection Import: " << (hitGroup->IntersectionShaderImport ? hitGroup->IntersectionShaderImport : L"[none]") << L"\n";
			break;
		}
		}
		wstr << L"|--------------------------------------------------------------------\n";
	}
	wstr << L"\n";
	OutputDebugStringW(wstr.str().c_str());
}

void RaytracingUtility::DispatchRays(CRayTracingPipelineStateObject* pRaytracingPipelineState, const D3D12_DISPATCH_RAYS_DESC* pDesc, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pRaytracingPipelineState && pDesc && pComputeCommandList);
	CComPtr<ID3D12GraphicsCommandList4> RaytracingCommandList;
	QueryRaytracingCommandList(pComputeCommandList, RaytracingCommandList);
	pComputeCommandList->CommitCachedDispatchResourcesManually();
	RaytracingCommandList->SetPipelineState1(pRaytracingPipelineState->GetD3D12RayTracingStateObject());
	RaytracingCommandList->DispatchRays(pDesc);
}

void RaytracingUtility::DispatchRays(CRayTracingPipelineStateObject* pRaytracingPipelineState, CRaytracingDispatchRaysDescriptor* pDesc, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pDesc);
	DispatchRays(pRaytracingPipelineState, pDesc->m_pDispatchRaysDesc, pComputeCommandList);
}