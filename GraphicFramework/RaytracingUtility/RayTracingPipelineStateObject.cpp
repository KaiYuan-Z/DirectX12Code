#include "RayTracingPipelineStateObject.h"
#include "../Core/GraphicsCore.h"
#include "RaytracingPipelineUtility.h"

using namespace RaytracingUtility;


CRayTracingPipelineStateObject::CRayTracingPipelineStateObject() : m_pRaytracingStateObjectDesc(nullptr)
{
}

CRayTracingPipelineStateObject::~CRayTracingPipelineStateObject()
{
}

void CRayTracingPipelineStateObject::ResetStateObject()
{
	if (m_pRaytracingStateObjectDesc) delete m_pRaytracingStateObjectDesc;
	m_pRaytracingStateObjectDesc = new CD3D12_STATE_OBJECT_DESC(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);
}

void CRayTracingPipelineStateObject::SetShaderProgram(UINT ProgramSize, const void* pProgramData, const std::vector<std::wstring>& ExportNameSet)
{
	THROW_MSG_IF_FALSE(m_pRaytracingStateObjectDesc != nullptr, L"You should call [ResetStateObject] before calling this function.\n");

	_ASSERTE(pProgramData && ProgramSize > 0);
	auto DxilLib = m_pRaytracingStateObjectDesc->CreateSubobject<CD3D12_DXIL_LIBRARY_SUBOBJECT>();
	auto ShaderCode = CD3DX12_SHADER_BYTECODE(pProgramData, ProgramSize);
	DxilLib->SetDXILLibrary(&ShaderCode);

	for (size_t i = 0; i < ExportNameSet.size(); i++) DxilLib->DefineExport(ExportNameSet[i].c_str());
}

void CRayTracingPipelineStateObject::SetHitGroup(const SHitGroupDesc& HitGroupDesc)
{
	THROW_MSG_IF_FALSE(m_pRaytracingStateObjectDesc != nullptr, L"You should call [ResetStateObject] before calling this function.\n");

	auto HitGroup = m_pRaytracingStateObjectDesc->CreateSubobject<CD3D12_HIT_GROUP_SUBOBJECT>();
	HitGroup->SetHitGroupType(HitGroupDesc.HitGroupType);
	HitGroup->SetHitGroupExport(HitGroupDesc.HitGroupName.c_str());
	if (HitGroupDesc.AnyHitShaderName.length() > 0) HitGroup->SetAnyHitShaderImport(HitGroupDesc.AnyHitShaderName.c_str());
	if (HitGroupDesc.ClosestHitShaderName.length() > 0) HitGroup->SetClosestHitShaderImport(HitGroupDesc.ClosestHitShaderName.c_str());
	if (HitGroupDesc.IntersectionShaderName.length() > 0) HitGroup->SetIntersectionShaderImport(HitGroupDesc.IntersectionShaderName.c_str());
}

void CRayTracingPipelineStateObject::SetShaderConfig(const SShaderConfigDesc& ShaderConfigDesc, const std::vector<std::wstring>& AssociatedShaderNameSet)
{
	THROW_MSG_IF_FALSE(m_pRaytracingStateObjectDesc != nullptr, L"You should call [ResetStateObject] before calling this function.\n");

	auto ShaderConfig = m_pRaytracingStateObjectDesc->CreateSubobject<CD3D12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	ShaderConfig->Config(ShaderConfigDesc.PayloadSize, ShaderConfigDesc.AttributeSize);

	auto ShaderConfigAssociation = m_pRaytracingStateObjectDesc->CreateSubobject<CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
	ShaderConfigAssociation->SetSubobjectToAssociate(*ShaderConfig);
	for (size_t i = 0; i < AssociatedShaderNameSet.size(); i++) ShaderConfigAssociation->AddExport(AssociatedShaderNameSet[i].c_str());
}

void CRayTracingPipelineStateObject::SetGlobalRootSignature(const CRootSignature& BindMappings)
{
	THROW_MSG_IF_FALSE(m_pRaytracingStateObjectDesc != nullptr, L"You should call [ResetStateObject] before calling this function.\n");

	auto GlobalRootSignature = m_pRaytracingStateObjectDesc->CreateSubobject<CD3D12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	GlobalRootSignature->SetRootSignature(BindMappings.GetSignature());
}

void CRayTracingPipelineStateObject::SetLocalRootSignature(const CRootSignature& BindMappings, const std::vector<std::wstring>& AssociatedShaderNameSet)
{
	THROW_MSG_IF_FALSE(m_pRaytracingStateObjectDesc != nullptr, L"You should call [ResetStateObject] before calling this function.\n");

	auto LocalRootSignature = m_pRaytracingStateObjectDesc->CreateSubobject<CD3D12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
	LocalRootSignature->SetRootSignature(BindMappings.GetSignature());

	auto LocalRootSignatureAssociation = m_pRaytracingStateObjectDesc->CreateSubobject<CD3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
	LocalRootSignatureAssociation->SetSubobjectToAssociate(*LocalRootSignature);
	for (size_t i = 0; i < AssociatedShaderNameSet.size(); i++) LocalRootSignatureAssociation->AddExport(AssociatedShaderNameSet[i].c_str());
}

void CRayTracingPipelineStateObject::SetMaxRecursionDepth(UINT MaxRecursionDepth)
{
	THROW_MSG_IF_FALSE(m_pRaytracingStateObjectDesc != nullptr, L"You should call [ResetStateObject] before calling this function.\n");

	// PERFOMANCE TIP: Set max recursion depth as low as needed as drivers may apply optimization strategies for low recursion depths.
	auto PipelineConfig = m_pRaytracingStateObjectDesc->CreateSubobject<CD3D12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	PipelineConfig->Config(MaxRecursionDepth);
}

void CRayTracingPipelineStateObject::FinalizeStateObject()
{
	THROW_MSG_IF_FALSE(m_pRaytracingStateObjectDesc != nullptr, L"You should call [ResetStateObject] before calling this function.\n");

	ID3D12Device* pDevice = GraphicsCore::GetD3DDevice();
	_ASSERTE(pDevice);
	CComPtr<ID3D12Device5> DxrDevice;
	RaytracingUtility::QueryRaytracingDevice(pDevice, DxrDevice);
	THROW_MSG_IF_FAILED(DxrDevice->CreateStateObject(*m_pRaytracingStateObjectDesc, IID_PPV_ARGS(&m_RayTracingStateObject)), L"Couldn't create DirectX Raytracing state object.\n");

	delete m_pRaytracingStateObjectDesc;
	m_pRaytracingStateObjectDesc = nullptr;
}

void* RaytracingUtility::CRayTracingPipelineStateObject::QueryShaderIdentifier(const std::wstring& ShaderName)
{
	CComPtr<ID3D12StateObjectPropertiesPrototype> StateObjectProperties;
	THROW_MSG_IF_FAILED(m_RayTracingStateObject.QueryInterface(&StateObjectProperties), L"Couldn't query shader identifier.\n");
	return StateObjectProperties->GetShaderIdentifier(ShaderName.c_str());
}
