#include "RayTracingAO.h"
#include "../../GraphicFramework/Core/Utility.h"
#include "../../GraphicFramework/Core/RootSignature.h"
#include "../../GraphicFramework/Core/GraphicsCore.h"
#include "../../GraphicFramework/Core/GraphicsCommon.h"
#include "../../GraphicFramework/Core/CommandManager.h"
#include "../../GraphicFramework/RaytracingUtility/RayTracingInstanceDesc.h"
#include "../../GraphicFramework/RaytracingUtility/RaytracingPipelineUtility.h"

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

	m_pCamera->OnKeyDown(key);
}

void CRayTracingAO::OnMouseMove(UINT x, UINT y)
{
	m_pCamera->OnMouseMove(x, y);
}

void CRayTracingAO::OnRightButtonDown(UINT x, UINT y)
{
	m_pCamera->OnMouseButtonDown(x, y);
}

// Update frame-based values.
void CRayTracingAO::OnUpdate()
{
	m_Scene.OnFrameUpdate();
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
	pComputeCommandList->TransitionResource(m_OutputBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pComputeCommandList->SetRootSignature(m_RayTracingRootSignature);
	m_SceneRenderHelper.SetGlobalRootParameter(pComputeCommandList, false);
	pComputeCommandList->SetDynamicConstantBufferView(m_UserRootParmStartIndex, sizeof(m_CBRayGen), &m_CBRayGen);
	pComputeCommandList->SetDynamicDescriptor(m_UserRootParmStartIndex + 1, 0, m_OutputBuffer.GetUAV());
	RaytracingUtility::DispatchRays(&m_RayTracingStateObject, &m_DispatchRaysDesc, pComputeCommandList);

	//
	// Show Output Image
	//
	m_ImageEffect.Render(kRED, &m_OutputBuffer, pGraphicsCommandList);

	//
	// Finish CommandList And Present
	//
	GraphicsCore::FinishCommandListAndPresent(pCommandList);
}

void CRayTracingAO::OnPostProcess()
{
	m_Scene.OnFramePostProcess();
}

void CRayTracingAO::__InitializeCommonResources()
{
	m_Scene.LoadScene(L"scene.txt");
	m_pCamera = m_Scene.GetCamera(0)->QueryWalkthroughCamera();
}

void CRayTracingAO::__InitializeRasterizerResources()
{
	m_ImageEffect.Init(1.0f);
}

void CRayTracingAO::__InitializeRayTracingResources()
{	
	_ASSERTE(RaytracingUtility::IsDxrSupported(GraphicsCore::GetD3DDevice()));

	m_Scene.InitRaytracingScene();
	m_Scene.AddTopLevelAccelerationStructure(L"RtSceneTLA", true, 2);
	m_SceneRenderHelper.InitRender(&m_Scene, 0);
	
	// Build raytracing root signature.
	m_UserRootParmStartIndex = m_SceneRenderHelper.GetRootParameterCount();
	m_RayTracingRootSignature.Reset(m_UserRootParmStartIndex + 2, m_SceneRenderHelper.GetStaticSamplerCount());
	m_SceneRenderHelper.ConfigurateGlobalRootSignature(m_RayTracingRootSignature);
	m_RayTracingRootSignature[m_UserRootParmStartIndex].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_ALL);// (b0)RayGenCB
	m_RayTracingRootSignature[m_UserRootParmStartIndex + 1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);// (u0)g_Output
	m_RayTracingRootSignature.Finalize(L"RayTracingAORoot");

	// Bulid raytracing local root signature.
	m_SceneRenderHelper.BuildHitProgLocalRootSignature(m_RaytracingLocalRootSignature);

	// Create raytrcing output buffer.
	m_OutputBuffer.Create2D(L"Output Buffer", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), 1, DXGI_FORMAT_R32_FLOAT);

	// Build raytracing state object.
	m_RayTracingStateObject.ResetStateObject();
	// ---Shader Program---
	m_RayTracingStateObject.SetShaderProgram(sizeof(g_pAoTracing), g_pAoTracing, { L"ScreenRayGen", L"CameraRayClosestHit", L"AoClosestHit", L"AnyHit", L"Miss" });
	// ---Shader Config---
	RaytracingUtility::SShaderConfigDesc ShaderConfigDesc;
	ShaderConfigDesc.AttributeSize = sizeof(XMFLOAT2);
	ShaderConfigDesc.PayloadSize = (sizeof(float) + sizeof(UINT));
	m_RayTracingStateObject.SetShaderConfig(ShaderConfigDesc, { L"ScreenRayGen", L"CameraRayClosestHit", L"AoClosestHit", L"AnyHit", L"Miss" });
	// ---Hit Group---
	RaytracingUtility::SHitGroupDesc CameraRayHitGroup;
	CameraRayHitGroup.HitGroupName = L"CameraRayHitGroup";
	CameraRayHitGroup.ClosestHitShaderName = L"CameraRayClosestHit";
	CameraRayHitGroup.AnyHitShaderName = L"AnyHit";
	m_RayTracingStateObject.SetHitGroup(CameraRayHitGroup);
	RaytracingUtility::SHitGroupDesc AoHitGroup;
	AoHitGroup.HitGroupName = L"AoHitGroup";
	AoHitGroup.ClosestHitShaderName = L"AoClosestHit";
	AoHitGroup.AnyHitShaderName = L"AnyHit";
	m_RayTracingStateObject.SetHitGroup(AoHitGroup);
	// ---Global Root Signature---
	m_RayTracingStateObject.SetGlobalRootSignature(m_RayTracingRootSignature);
	// ---Local Root Signature---
	m_RayTracingStateObject.SetLocalRootSignature(m_RaytracingLocalRootSignature, { L"CameraRayHitGroup", L"AoHitGroup" });
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
	m_SceneRenderHelper.BuildMeshHitShaderTable(0, &m_RayTracingStateObject, { L"CameraRayHitGroup", L"AoHitGroup" }, m_HitGroupShaderTable);

	// Bulid dispatch rays descriptor.
	m_DispatchRaysDesc = MakeDispatchRaysDesc(GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), &m_RayGenShaderTable, &m_MissShaderTable, &m_HitGroupShaderTable);
}

void CRayTracingAO::__UpdateGUI()
{
	ImGui::SetNextWindowSize(ImVec2(450, 220));
	ImGui::Begin("RayTracingAO");
	ImGui::SliderFloat("AoRadius", &m_CBRayGen.AORadius, 0.0f, 50.0f);
	ImGui::SliderFloat("MinT", &m_CBRayGen.MinT, 0.0f, 2.0f, "%.3f", 2.0f);
	ImGui::SliderInt("AONumRays", &m_CBRayGen.AONumRays, 1, 256);
	ImGui::SliderInt("PixelNumRays", &m_CBRayGen.PixelNumRays, 1, 64);
	const char* items[] = { "2", "3", "5", "7", "11", "13" };
	static int CurHaltonBaseIndex1 = 2;
	static int CurHaltonBaseIndex2 = 3;
	ImGui::Combo("HaltonBase1", &CurHaltonBaseIndex1, items, 6);
	ImGui::Combo("HaltonBase2", &CurHaltonBaseIndex2, items, 6);
	m_CBRayGen.HaltonBase1 = std::atoi(items[CurHaltonBaseIndex1]);
	m_CBRayGen.HaltonBase2 = std::atoi(items[CurHaltonBaseIndex2]);
	ImGui::InputInt("HaltonIndexStep", &m_CBRayGen.HaltonIndexStep, 1, 128);
	ImGui::InputInt("HaltonStartIndex", &m_CBRayGen.HaltonStartIndex, 1, 1024);
	ImGui::End();
}