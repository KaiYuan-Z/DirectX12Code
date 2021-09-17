#include "RayTracingAO.h"
#include "../../GraphicFramework/RaytracingUtility/RayTracingInstanceDesc.h"
#include "../../GraphicFramework/RaytracingUtility/RaytracingPipelineUtility.h"
#include "../../GraphicFramework/Core/Utility.h"
#include "../../GraphicFramework/Core/RootSignature.h"
#include "../../GraphicFramework/Core/GraphicsCore.h"
#include "../../GraphicFramework/Core/GraphicsCommon.h"

#include "CompiledShaders/AoTracing.h"

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

CRayTracingAO::CRayTracingAO(std::wstring name) : CDXSample(name)
{
}

CRayTracingAO::~CRayTracingAO()
{
}

void CRayTracingAO::OnInit()
{
	//
	// Register GUI Callbak Function
	//
	GraphicsCore::RegisterGuiCallbakFunction(std::bind(&CRayTracingAO::__UpdateGUI, this));

	//
	// Init Common Resources
	//
	__InitializeCommonResources();

	//
	// Init Rasterizer Resources
	//
	__InitializeRasterizerResources();

	//
	// Init RayTracing Resources
	//
	__InitializeRayTracingResources();

	GraphicsCore::IdleGPU();
}


void CRayTracingAO::OnKeyDown(UINT8 key)
{
	/*switch (key)
	{
	default:_ASSERT(false); break;
	}*/

	m_Camera.OnKeyDown(key);
}

void CRayTracingAO::OnMouseMove(UINT x, UINT y)
{
	m_Camera.OnMouseMove(x, y);
}

void CRayTracingAO::OnRightButtonDown(UINT x, UINT y)
{
	m_Camera.OnMouseButtonDown(x, y);
}

// Update frame-based values.
void CRayTracingAO::OnUpdate()
{
	if (m_Camera.IsCameraModified())
	{
		m_SimpleAccumulationEffect.ResetAccumCount();

		m_Camera.OnFrameUpdate();
		m_CBScene.CameraPosition = XMFLOAT4(m_Camera.GetPositionXMF3().x, m_Camera.GetPositionXMF3().y, m_Camera.GetPositionXMF3().z, 1.0f);
		m_CBScene.ViewProj = m_Camera.GetViewXMM()*m_Camera.GetProjectXMM();
		m_CBScene.InvViewProj = XMMatrixInverse(nullptr, m_CBScene.ViewProj);
	}

	m_CBScene.FrameCount = (UINT)GraphicsCore::GetFrameID();
}

// Render the scene.
void CRayTracingAO::OnRender()
{
	if (!GraphicsCore::IsWindowVisible())
	{
		return;
	}

	//
	// Get CommandList
	//
	CCommandList* pCommandList = GraphicsCore::BeginCommandList();
	_ASSERTE(pCommandList);
	CComputeCommandList* pComputeCommandList = pCommandList->QueryComputeCommandList();
	_ASSERTE(pComputeCommandList);
	CGraphicsCommandList* pGraphicsCommandList = pCommandList->QueryGraphicsCommandList();
	_ASSERTE(pGraphicsCommandList);

	//
	// Raytracing Rendering
	//
	pComputeCommandList->TransitionResource(m_AoOutputBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pComputeCommandList->SetRootSignature(m_RayTracingRootSignature);
	pComputeCommandList->SetDynamicConstantBufferView(0, sizeof(m_CBRayGen), &m_CBRayGen);
	pComputeCommandList->SetDynamicConstantBufferView(1, sizeof(m_CBScene), &m_CBScene);
	pComputeCommandList->SetBufferSRV(2, m_TopLevelAccelerationStructure.GetAccelerationStructureBufferData());
	D3D12_CPU_DESCRIPTOR_HANDLE ModelDataHandles[] = { m_Model.GetIndexBuffer().GetSRV(), m_Model.GetVertexBuffer().GetSRV() };
	pComputeCommandList->SetDynamicDescriptors(3, 0, 2, ModelDataHandles);
	pComputeCommandList->SetDynamicDescriptor(4, 0, m_ModelMeshInfoBuffer.GetSRV());
	pComputeCommandList->SetDynamicDescriptor(5, 0, m_AoOutputBuffer.GetUAV());
	RaytracingUtility::DispatchRays(&m_RayTracingStateObject, &m_DispatchRaysDesc, pComputeCommandList);

	//
	// Accumulate Raytracing Result
	//
	m_SimpleAccumulationEffect.Render(&m_DoubleAccumBuffer.GetBackBuffer(), &m_AoOutputBuffer, &m_DoubleAccumBuffer.GetFrontBuffer(), pGraphicsCommandList);

	//
	// Show Output Image
	//
	m_ImageEffect.Render(kRED, &m_DoubleAccumBuffer.GetFrontBuffer(), pGraphicsCommandList);
	
	//
	// Finish CommandList And Present
	//
	GraphicsCore::FinishCommandListAndPresent(pCommandList);

	//
	// Swap Accumulation Output
	//
	m_DoubleAccumBuffer.SwapBuffer();
}

void CRayTracingAO::__InitializeCommonResources()
{
	// Init camera.
	m_Camera.SetLens(XMConvertToRadians(45), float(GraphicsCore::GetWindowWidth()) / float(GraphicsCore::GetWindowHeight()), 0.1f, 1500.0f);
	m_Camera.SetLookAt(XMFLOAT3(-64.752838f, 98.669799f, 11.468944f), XMFLOAT3(80.0f, 50.0f, -100.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_Camera.SetMoveStep(1.0f);
	m_Camera.SetRotateStep(0.0125f);

	// Init Model.
	if (m_Model.Load("room1.h3d"))
	{
		_ASSERTE(m_Model.GetHeader().meshCount > 0);
	}
	else
	{
		_ASSERTE(false);
	}
}

void CRayTracingAO::__InitializeRasterizerResources()
{
	m_SimpleAccumulationEffect.Init(DXGI_FORMAT_R32_FLOAT, GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), m_MaxAccumCount);
	m_ImageEffect.Init(1.0f);

	m_DoubleAccumBuffer.Create2D(L"Accum Buffer", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), 1, DXGI_FORMAT_R32_FLOAT);
}

void CRayTracingAO::__InitializeRayTracingResources()
{
	_ASSERTE(RaytracingUtility::IsDxrSupported(GraphicsCore::GetD3DDevice()));

	// Build raytracing root signature.
	m_RayTracingRootSignature.Reset(6, 1);
	m_RayTracingRootSignature.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerLinearWrapDesc);
	m_RayTracingRootSignature[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_ALL);// (b0)RayGenCB
	m_RayTracingRootSignature[1].InitAsConstantBuffer(1, 0, D3D12_SHADER_VISIBILITY_ALL);// (b1)SceneCB
	m_RayTracingRootSignature[2].InitAsBufferSRV(0, 0, D3D12_SHADER_VISIBILITY_ALL);// (t0)g_scene
	m_RayTracingRootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0, D3D12_SHADER_VISIBILITY_ALL);// (t1)g_Indeces (t2)g_Vertices
	m_RayTracingRootSignature[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, 0, D3D12_SHADER_VISIBILITY_ALL);// (t3)g_MeshInfo
	m_RayTracingRootSignature[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);// (u0)g_Output
	m_RayTracingRootSignature.Finalize(L"RayTracingAORoot");

	// Build local raytracing root signature for camera ray hit group.
	m_RaytracingLocalRootSignature.Reset(1, 0);
	m_RaytracingLocalRootSignature[0].InitAsConstants(2, 1, 0);// (b2)Material
	m_RaytracingLocalRootSignature.Finalize(L"RayTracingAOLocalRoot", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

	// Create raytrcing output buffer.
	m_AoOutputBuffer.Create2D(L"Ao Output Buffer", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), 1, DXGI_FORMAT_R32_FLOAT);

	// Build raytrcing shader buffer.
	const UINT MeshCount = m_Model.GetHeader().meshCount;
	std::vector<SModelMeshInfo> ModelMeshInfoSet(MeshCount);
	for (UINT i = 0; i < MeshCount; i++)
	{
		ModelMeshInfoSet[i].IndexOffsetBytes = m_Model.GetMesh(i).indexDataByteOffset;
		ModelMeshInfoSet[i].VertexOffsetBytes = m_Model.GetMesh(i).vertexDataByteOffset;
		ModelMeshInfoSet[i].VertexStride = m_Model.GetMesh(i).vertexStride;
	}
	m_ModelMeshInfoBuffer.Create(L"Model Mesh Info Buffer", MeshCount, sizeof(SModelMeshInfo), ModelMeshInfoSet.data());
	m_CBRayGen.AORadius = 20.0f;
	m_CBRayGen.AONumRays = 8;
	m_CBRayGen.PixelNumRays = 1;
	m_CBRayGen.MinT = 0.001f;

	// Build acceleration structure.
	m_BottomLevelAccelerationStructure.Create(m_Model);
	std::vector<RaytracingUtility::SRayTracingInstanceDesc> RayTracingInstanceDesc = { RaytracingUtility::SRayTracingInstanceDesc(m_BottomLevelAccelerationStructure.GetAccelerationStructureGpuVirtualAddr()) };
	m_TopLevelAccelerationStructure.Create(RayTracingInstanceDesc);

	// Build raytracing state object.
	m_RayTracingStateObject.ResetStateObject();
	// ---Shader Program---
	m_RayTracingStateObject.SetShaderProgram(sizeof(g_pAoTracing), g_pAoTracing, { L"ScreenRayGen" , L"CameraRayClosestHit" , L"AoClosestHit" , L"Miss" });
	// ---Shader Config---
	RaytracingUtility::SShaderConfigDesc ShaderConfigDesc;
	ShaderConfigDesc.AttributeSize = sizeof(XMFLOAT2);
	ShaderConfigDesc.PayloadSize = (sizeof(float) + sizeof(UINT));
	m_RayTracingStateObject.SetShaderConfig(ShaderConfigDesc, { L"ScreenRayGen" , L"CameraRayClosestHit" , L"AoClosestHit" , L"Miss" });
	// ---Hit Group---
	RaytracingUtility::SHitGroupDesc CameraRayHitGroup;
	CameraRayHitGroup.HitGroupName = L"CameraRayHitGroup";
	CameraRayHitGroup.ClosestHitShaderName = L"CameraRayClosestHit";
	m_RayTracingStateObject.SetHitGroup(CameraRayHitGroup);
	RaytracingUtility::SHitGroupDesc AoHitGroup;
	AoHitGroup.HitGroupName = L"AoHitGroup";
	AoHitGroup.ClosestHitShaderName = L"AoClosestHit";
	m_RayTracingStateObject.SetHitGroup(AoHitGroup);
	// ---Global Root Signature---
	m_RayTracingStateObject.SetGlobalRootSignature(m_RayTracingRootSignature);
	// ---Local Root Signature---
	m_RayTracingStateObject.SetLocalRootSignature(m_RaytracingLocalRootSignature, { L"CameraRayHitGroup" });
	// ---Pipeline Config---
	m_RayTracingStateObject.SetMaxRecursionDepth(2);
	m_RayTracingStateObject.FinalizeStateObject();

	// Build raytracing shader tables.
	// ---RayGenShaderTable---
	m_RayGenShaderTable.ResetShaderTable(1);
	m_RayGenShaderTable[0].ResetShaderRecord(m_RayTracingStateObject.QueryShaderIdentifier(L"ScreenRayGen"));
	m_RayGenShaderTable[0].FinalizeShaderRecord();
	m_RayGenShaderTable.FinalizeShaderTable();
	// ---MissShaderTable---
	m_MissShaderTable.ResetShaderTable(1);
	m_MissShaderTable[0].ResetShaderRecord(m_RayTracingStateObject.QueryShaderIdentifier(L"Miss"));
	m_MissShaderTable[0].FinalizeShaderRecord();
	m_MissShaderTable.FinalizeShaderTable();
	// ---HitGroupShaderTable---
	UINT HitShaderRecordIndex = 0;
	m_HitGroupShaderTable.ResetShaderTable(MeshCount + 1);
	m_HitGroupShaderTable[HitShaderRecordIndex].ResetShaderRecord(m_RayTracingStateObject.QueryShaderIdentifier(L"AoHitGroup"));
	m_HitGroupShaderTable[HitShaderRecordIndex].FinalizeShaderRecord();
	HitShaderRecordIndex++;
	for (UINT i = 0; i < MeshCount; i++)
	{
		UINT MeshID = i;
		m_HitGroupShaderTable[HitShaderRecordIndex].ResetShaderRecord(m_RayTracingStateObject.QueryShaderIdentifier(L"CameraRayHitGroup"), 1);
		m_HitGroupShaderTable[HitShaderRecordIndex].SetLocalRoot32BitConstants(0, 1, &MeshID);
		m_HitGroupShaderTable[HitShaderRecordIndex].FinalizeShaderRecord();
		HitShaderRecordIndex++;
	}
	m_HitGroupShaderTable.FinalizeShaderTable();

	// Bulid dispatch rays descriptor.
	m_DispatchRaysDesc = MakeDispatchRaysDesc(GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), &m_RayGenShaderTable, &m_MissShaderTable, &m_HitGroupShaderTable);
}

void CRayTracingAO::__UpdateGUI()
{
	ImGui::SetNextWindowSize(ImVec2(450, 150));
	ImGui::Begin("TemporalRayTracingAO");
	ImGui::Text("Accumulate Frame = %d (MaxAccumFrame:%d)", m_SimpleAccumulationEffect.GetAccumCount(), m_MaxAccumCount);
	ImGui::SliderFloat("AoRadius", &m_CBRayGen.AORadius, 0.0f, 50.0f);
	ImGui::SliderFloat("MinT", &m_CBRayGen.MinT, 0.0f, 2.0f, "%.3f", 2.0f);
	ImGui::SliderInt("AONumRays", &m_CBRayGen.AONumRays, 1, 128);
	ImGui::SliderInt("PixelNumRays", &m_CBRayGen.PixelNumRays, 1, 16);
	ImGui::End();
}
