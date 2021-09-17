#include "RayTracingAO.h"
#include "../../GraphicFramework/Core/Utility.h"
#include "../../GraphicFramework/Core/RootSignature.h"
#include "../../GraphicFramework/Core/GraphicsCore.h"
#include "../../GraphicFramework/Core/GraphicsCommon.h"
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
	// Rasterizer Rendering
	//
	m_GBufferEffect.Render(pGraphicsCommandList);

	//
	// Raytracing Rendering
	//
	pComputeCommandList->TransitionResource(*m_GBufferEffect.GetCurPosBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	pComputeCommandList->TransitionResource(*m_GBufferEffect.GetCurNormBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);
	pComputeCommandList->SetRootSignature(m_RtGlobalRootSignature);
	m_SceneRenderHelper.SetGlobalRootParameter(pComputeCommandList, false);
	pComputeCommandList->SetDynamicConstantBufferView(m_UserRootParmStartIndex, sizeof(m_CBRayGen),&m_CBRayGen);
	pComputeCommandList->SetDynamicDescriptors(m_UserRootParmStartIndex + 1, 0, 1, &m_GBufferEffect.GetCurPosBuffer()->GetSRV());
	pComputeCommandList->SetDynamicDescriptors(m_UserRootParmStartIndex + 2, 0, 1, &m_GBufferEffect.GetCurNormBuffer()->GetSRV());
	pComputeCommandList->SetDynamicDescriptor(m_UserRootParmStartIndex + 3, 0, m_OutputBuffer.GetUAV());
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
	SGeometryOnlyGBufferConfig GBufferConfig;
	GBufferConfig.OpenBackFaceCulling = false;
	GBufferConfig.SceneRenderConfig.ActiveCameraIndex = 0;
	GBufferConfig.SceneRenderConfig.pScene = &m_Scene;
	GBufferConfig.Width = GraphicsCore::GetWindowWidth();
	GBufferConfig.Height = GraphicsCore::GetWindowHeight();
	GBufferConfig.DepthStencilBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	m_GBufferEffect.Init(GBufferConfig);
	m_ImageEffect.Init(1.0f);
}

void CRayTracingAO::__InitializeRayTracingResources()
{
	_ASSERTE(RaytracingUtility::IsDxrSupported(GraphicsCore::GetD3DDevice()));
	
	m_Scene.InitRaytracingScene();
	m_Scene.AddTopLevelAccelerationStructure(L"RtSceneTLA", false, 1);
	m_SceneRenderHelper.InitRender(&m_Scene, 0, 0); 

	// Build raytracing global root signature.
	m_RtGlobalRootSignature.Reset(m_SceneRenderHelper.GetRootParameterCount() + 4, m_SceneRenderHelper.GetStaticSamplerCount());
	m_SceneRenderHelper.ConfigurateGlobalRootSignature(m_RtGlobalRootSignature);
	m_UserRootParmStartIndex = m_SceneRenderHelper.GetRootParameterCount();
	m_RtGlobalRootSignature[m_UserRootParmStartIndex].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_ALL);// (b0)RayGenCB
	m_RtGlobalRootSignature[m_UserRootParmStartIndex + 1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0, D3D12_SHADER_VISIBILITY_ALL);// (t1)g_Pos
	m_RtGlobalRootSignature[m_UserRootParmStartIndex + 2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_SHADER_VISIBILITY_ALL);// (t2)g_Norm
	m_RtGlobalRootSignature[m_UserRootParmStartIndex + 3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);// (u0)g_Output
	m_RtGlobalRootSignature.Finalize(L"RayTracingAO");

	// Bulid raytracing local root signature.
	m_SceneRenderHelper.BuildHitProgLocalRootSignature(m_RtLocalRootSignature);

	// Create raytrcing output buffer.
	m_OutputBuffer.Create2D(L"Output Buffer", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), 1, DXGI_FORMAT_R32_FLOAT);

	// Build raytrcing constant buffer.
	m_CBRayGen.AORadius = 12.0f;
	m_CBRayGen.AONumRays = 16;
	m_CBRayGen.MinT = 0.01f;

	// Build raytracing state object.
	m_RayTracingStateObject.ResetStateObject();
	// ---Shader Program---
	m_RayTracingStateObject.SetShaderProgram(sizeof(g_pAoTracing), g_pAoTracing, { L"AoRayGen", L"AoClosestHit", L"AoAnyHit", L"AoMiss" });
	// ---Shader Config---
	RaytracingUtility::SShaderConfigDesc ShaderConfigDesc;
	ShaderConfigDesc.AttributeSize = sizeof(XMFLOAT2);
	ShaderConfigDesc.PayloadSize = (sizeof(float));
	m_RayTracingStateObject.SetShaderConfig(ShaderConfigDesc, { L"AoRayGen" , L"AoClosestHit", L"AoAnyHit", L"AoMiss" });
	// ---Hit Group---
	RaytracingUtility::SHitGroupDesc AoHitGroup;
	AoHitGroup.HitGroupName = L"AoHitGroup";
	AoHitGroup.ClosestHitShaderName = L"AoClosestHit";
	AoHitGroup.AnyHitShaderName = L"AoAnyHit";
	m_RayTracingStateObject.SetHitGroup(AoHitGroup);
	// ---Global Root Signature---
	m_RayTracingStateObject.SetGlobalRootSignature(m_RtGlobalRootSignature);
	// ---Local Root Signature---
	m_RayTracingStateObject.SetLocalRootSignature(m_RtLocalRootSignature, { L"AoHitGroup" });
	// ---Pipeline Config---
	m_RayTracingStateObject.SetMaxRecursionDepth(1);
	m_RayTracingStateObject.FinalizeStateObject();

	// Build raytracing shader tables.
	m_RayGenShaderTable.ResetShaderTable(1);
	m_RayGenShaderTable[0].ResetShaderRecord(m_RayTracingStateObject.QueryShaderIdentifier(L"AoRayGen"));
	m_RayGenShaderTable[0].FinalizeShaderRecord();
	m_RayGenShaderTable.FinalizeShaderTable();
	m_MissShaderTable.ResetShaderTable(1);
	m_MissShaderTable[0].ResetShaderRecord(m_RayTracingStateObject.QueryShaderIdentifier(L"AoMiss"));
	m_MissShaderTable[0].FinalizeShaderRecord();
	m_MissShaderTable.FinalizeShaderTable();
	m_SceneRenderHelper.BuildMeshHitShaderTable(0, &m_RayTracingStateObject, { L"AoHitGroup" }, m_HitShaderTable);

	// Bulid dispatch rays descriptor.
	m_DispatchRaysDesc = MakeDispatchRaysDesc(GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), &m_RayGenShaderTable, &m_MissShaderTable, &m_HitShaderTable);
}

void CRayTracingAO::__UpdateGUI()
{
	ImGui::SetNextWindowSize(ImVec2(450, 150));
	ImGui::Begin("RayTracingAO");
	ImGui::SliderFloat("AoRadius", &m_CBRayGen.AORadius, 0.0f, 50.0f);
	ImGui::SliderFloat("MinT", &m_CBRayGen.MinT, 0.0f, 2.0f,"%.3f", 2.0f);
	ImGui::SliderInt("AONumRays", &m_CBRayGen.AONumRays, 1, 128);
	ImGui::Checkbox("OpenCosineSample", &m_CBRayGen.OpenCosineSample);
	ImGui::End();
}