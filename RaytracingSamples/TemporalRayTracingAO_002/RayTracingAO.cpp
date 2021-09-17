#include "RayTracingAO.h"
#include "../../GraphicFramework/Core/Utility.h"
#include "../../GraphicFramework/Core/RootSignature.h"
#include "../../GraphicFramework/Core/GraphicsCore.h"
#include "../../GraphicFramework/Core/GraphicsCommon.h"
#include "../../GraphicFramework/RaytracingUtility/RayTracingInstanceDesc.h"
#include "../../GraphicFramework/RaytracingUtility/RaytracingPipelineUtility.h"
#include "../../GraphicFramework/Core/GpuTimeManager.h"

#include "CompiledShaders/AoTracing.h"
#include "CompiledShaders/ModelViewerPS.h"
#include "CompiledShaders/ModelViewerVS.h"

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

	//
	// Init Render Buffers
	//
	__InitRenderBuffers();


	GraphicsCore::IdleGPU();
}


void CRayTracingAO::OnKeyDown(UINT8 key)
{
	switch (key)
	{
	case VK_F1:m_DisplayMode = NewAoMap; break;
	case VK_F2:m_DisplayMode = AuccumedAoMap; break;
	case VK_F3:m_DisplayMode = FinalAoMap; break;
	case 'm':
	case 'M':m_CarMove = !m_CarMove; break;
	case 'n':
	case 'N':m_DebugInfo = !m_DebugInfo; break; 
	case 'b':
	case 'B':m_ShowAoBounding = !m_ShowAoBounding; break;
	case 'o':
	case 'O':m_EnableAo = !m_EnableAo; break;
	case 'i':
	case 'I':m_EnableShadow = !m_EnableShadow; break;
	case 'g':
	case 'G':m_EabbleGrassAnim = !m_EabbleGrassAnim; break;
	case '1':__SetConfigSet(m_Config1); break;
	//case '3':__SetConfigSet(m_Config2); break;
	case '2':__SetConfigSet(m_Config3); break;
	//case '4':__SetConfigSet(m_Config4); break;
	case '7':__SetCameraParamter(m_CamParam1); break;
	case '8':__SetCameraParamter(m_CamParam2); break;
	}

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
	float delta = (float)GraphicsCore::GetTimer().GetElapsedSeconds();

	if (m_CarMove)
	{
		CMotionManager::sDoMotions(delta);
	}

	m_Scene.OnFrameUpdate();

	m_CalcAoDiffEffect.Update(m_pCamera, m_CBRayGen.AORadius, m_CBRayGen.MinT);
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
	// Sky Color
	//
	m_SkyEffect.Render(true, &m_SkyColorBuffer, pGraphicsCommandList);

	//
	// GBuffer
	//
	m_GBufferEffect.Render(pGraphicsCommandList, GraphicsCommon::EShaderingType::kPbr);

	//
	// Shadow
	//
	if (m_EnableShadow)
	{
		m_ShadowMapEffect.Render(pGraphicsCommandList);
	}
	
	//if(m_EnableAo)
	{
		m_MarkDynamic.Render(m_CBRayGen.AORadius, m_GBufferEffect.GetCurDepthBuffer(), pGraphicsCommandList);
		
		//
		// Calculate Accum Map
		//
		m_AccumEffect.CalculateAccumMap(m_GBufferEffect.GetPreLinearDepthBuffer(),
			m_GBufferEffect.GetPreNormBuffer(),
			m_GBufferEffect.GetCurLinearDepthBuffer(),
			m_GBufferEffect.GetCurNormBuffer(),
			m_GBufferEffect.GetCurDdxyBuffer(),
			m_GBufferEffect.GetCurMotionBuffer(),
			m_GBufferEffect.GetCurIdBuffer(),
			&m_MarkDynamic.GetDynamicMarkMap(),
			pComputeCommandList);

		//
		// Calculate Ao Diff
		//
		m_CalcAoDiffEffect.CalcAoDiff(&m_HaltonIndexBuffer,
			&m_PreSampleCacheBuffer,
			m_GBufferEffect.GetPreIdBuffer(),
			m_GBufferEffect.GetCurLinearDepthBuffer(),
			m_GBufferEffect.GetCurNormBuffer(),
			m_GBufferEffect.GetCurDdxyBuffer(),
			m_GBufferEffect.GetCurMotionBuffer(),
			&m_AccumEffect.GetCurAccumMap(),
			pComputeCommandList);

		//
		// Raytracing Rendering
		//
		pComputeCommandList->TransitionResource(m_OutputBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
		pComputeCommandList->TransitionResource(m_PreSampleCacheBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
		pComputeCommandList->TransitionResource(m_HaltonIndexBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
		pComputeCommandList->TransitionResource(m_AccumEffect.GetCurAccumMap(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
		pComputeCommandList->TransitionResource(*m_GBufferEffect.GetCurPosBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
		pComputeCommandList->TransitionResource(*m_GBufferEffect.GetCurNormBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);

		pComputeCommandList->SetRootSignature(m_RayTracingRootSignature);

		m_SceneRenderHelper.SetGlobalRootParameter(pComputeCommandList);

		pComputeCommandList->SetDynamicConstantBufferView(m_UserRootParmStartIndex + 0, sizeof(m_CBRayGen), &m_CBRayGen);
		D3D12_CPU_DESCRIPTOR_HANDLE Srvs[2] = {
			m_GBufferEffect.GetCurIdBuffer()->GetSRV(),
			m_CalcAoDiffEffect.GetAoDiffMap().GetSRV()
		};
		pComputeCommandList->SetDynamicDescriptors(m_UserRootParmStartIndex + 1, 0, 2, Srvs);

		D3D12_CPU_DESCRIPTOR_HANDLE Uavs[4] = {
			m_OutputBuffer.GetUAV(),
			m_PreSampleCacheBuffer.GetUAV(),
			m_HaltonIndexBuffer.GetUAV(),
			m_AccumEffect.GetCurAccumMap().GetUAV()
		};
		pComputeCommandList->SetDynamicDescriptors(m_UserRootParmStartIndex + 2, 0, 4, Uavs);

		RaytracingUtility::DispatchRays(&m_RayTracingStateObject, &m_DispatchRaysDesc, pComputeCommandList);

		//
		// Accumulate Raytracing Result
		//
		m_AccumEffect.AccumulateFrame(m_CBRayGen.MaxAccumCount,
			&m_OutputBuffer,
			&m_CalcAoDiffEffect.GetAoDiffMap(),
			pComputeCommandList);

		//
		// Blur Output 
		//
		m_AtrousWaveletTransfromBlurEffect.BlurImage(1, 1,
			m_GBufferEffect.GetCurLinearDepthBuffer(),
			m_GBufferEffect.GetCurNormBuffer(),
			m_GBufferEffect.GetPreDdxyBuffer(),
			&m_AccumEffect.GetCurAccumFrame(),
			&m_BlurOutputBuffer, pComputeCommandList);
		pCommandList->TransitionResource(m_BlurOutputBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pCommandList->TransitionResource(m_AccumEffect.GetCurAccumFrame(), D3D12_RESOURCE_STATE_COPY_DEST);
		pCommandList->CopySubresource(m_AccumEffect.GetCurAccumFrame(), 0, m_BlurOutputBuffer, 0);

		m_BilateralBlurEffect.BlurImage(2, 5,
			m_GBufferEffect.GetCurLinearDepthBuffer(),
			m_GBufferEffect.GetCurNormBuffer(),
			m_GBufferEffect.GetCurDdxyBuffer(),
			&m_AccumEffect.GetCurAccumFrame(),
			&m_BlurOutputBuffer,
			pComputeCommandList);

		m_AccumEffect.MixBluredResult(&m_BlurOutputBuffer, &m_CalcAoDiffEffect.GetAoDiffMap(), pComputeCommandList);
	}

	//
	// Color Pass
	//
	pGraphicsCommandList->SetViewportAndScissor(GraphicsCore::GetScreenViewport(), GraphicsCore::GetScissorRect());

	pGraphicsCommandList->TransitionResource(m_ShadowMapEffect.GetShadowMapBuffer(), D3D12_RESOURCE_STATE_DEPTH_READ, false);
	pGraphicsCommandList->TransitionResource(m_AccumEffect.GetCurMixBlurFrame(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(m_SkyColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(m_ColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pGraphicsCommandList->TransitionResource(m_DepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	pGraphicsCommandList->ClearColor(m_ColorBuffer);
	pGraphicsCommandList->ClearDepth(m_DepthBuffer);
	
	pGraphicsCommandList->SetRootSignature(m_RootSignature);
	pGraphicsCommandList->SetPipelineState(m_PSO);
	pGraphicsCommandList->SetConstants(m_RootStartIndex, m_EnableAo, m_EnableShadow);
	pGraphicsCommandList->SetDynamicConstantBufferView(m_RootStartIndex + 1, sizeof(XMMATRIX), &m_ShadowMapEffect.GetShadowTransform());
	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[3] = { 
		m_AccumEffect.GetCurMixBlurFrame().GetSRV(), 
		m_ShadowMapEffect.GetShadowMapBuffer().GetDepthSRV(),
		m_SkyColorBuffer.GetSRV()
	};
	pGraphicsCommandList->SetDynamicDescriptors(m_RootStartIndex + 2, 0, 3, Srvs);
	pGraphicsCommandList->SetRenderTarget(m_ColorBuffer.GetRTV(), m_DepthBuffer.GetDSV());

	m_SceneRender.DrawQuad({
		m_GBufferEffect.GetCurPosBuffer(),
		m_GBufferEffect.GetCurNormBuffer(),
		m_GBufferEffect.GetCurDiffBuffer(),
		m_GBufferEffect.GetCurSpecBuffer(),
		m_GBufferEffect.GetCurIdBuffer() },
		pGraphicsCommandList);

	//
	// FXAA Pass
	//
	switch (m_DisplayMode)
	{
	case CRayTracingAO::NewAoMap:
		m_FxaaEffect.RenderFxaa(&m_OutputBuffer, &m_FinalOutputBuffer, pGraphicsCommandList);
		break;
	case CRayTracingAO::AuccumedAoMap:
		m_FxaaEffect.RenderFxaa(&m_AccumEffect.GetCurMixBlurFrame(), &m_FinalOutputBuffer, pGraphicsCommandList);
		break;
	case CRayTracingAO::FinalAoMap:
		m_FxaaEffect.RenderFxaa(&m_ColorBuffer, &m_FinalOutputBuffer, pGraphicsCommandList);
		break;
	default:
		break;
	}

	//
	// Show Output Image
	//
	switch (m_DisplayMode)
	{
	case CRayTracingAO::NewAoMap:
		m_ImageEffect.Render(kRED, &m_FinalOutputBuffer, pGraphicsCommandList);
		break;
	case CRayTracingAO::AuccumedAoMap:
		m_ImageEffect.Render(kRED, &m_FinalOutputBuffer, pGraphicsCommandList);
		break;
	case CRayTracingAO::FinalAoMap:
		m_ImageEffect.Render(kRGBA, &m_FinalOutputBuffer, pGraphicsCommandList);
		break;
	default:
		break;
	}

	if (m_DebugInfo)
	{
		m_ImageEffectDebugAccum.Render(kAccumMap, &m_AccumEffect.GetCurAccumMap(), pGraphicsCommandList);
		m_ImageEffectDebugAoDiff.Render(kRED, &m_CalcAoDiffEffect.GetAoDiffMap(), pGraphicsCommandList);
		
		if (m_ShowAoBounding)
		{
			m_ImageEffectDebugDynamicMark.Render(kDynamicMark, &m_MarkDynamic.GetDynamicMarkMap(), pGraphicsCommandList);
		}
	}

	//
	// Finish CommandList And Present
	//
	GraphicsCore::FinishCommandListAndPresent(pCommandList, false);
}

void CRayTracingAO::OnPostProcess()
{
	m_Scene.OnFramePostProcess();
}

void CRayTracingAO::__InitializeCommonResources()
{
	m_Scene.LoadScene(L"scene.txt");
	m_pCamera = m_Scene.GetCamera(0)->QueryWalkthroughCamera();

	const float maxDist = 50.0f;

	m_pBusInst1 = m_Scene.GetInstance(L"bus_Inst0");
	m_BusInst1Motion.InitMotionManager(m_pBusInst1, kForwardZ, maxDist, -maxDist, 7.0f);
	m_pBusInst2 = m_Scene.GetInstance(L"bus_Inst1");
	m_BusInst2Motion.InitMotionManager(m_pBusInst2, kForwardZ, -maxDist, maxDist, 7.0f);

	m_pCar1Inst1 = m_Scene.GetInstance(L"car1_Inst0");
	m_Car1Inst1Motion.InitMotionManager(m_pCar1Inst1, kForwardZ, maxDist, -maxDist, 10.0f);
	m_pCar1Inst2 = m_Scene.GetInstance(L"car1_Inst1");
	m_Car1Inst2Motion.InitMotionManager(m_pCar1Inst2, kForwardZ, -maxDist, maxDist, 10.0f);

	m_pUaz1Inst1 = m_Scene.GetInstance(L"uaz_Inst0");
	m_Uaz1Inst1Motion.InitMotionManager(m_pUaz1Inst1, kForwardZ, maxDist, -maxDist, 7.0f);
	m_pUaz1Inst2 = m_Scene.GetInstance(L"uaz_Inst1");
	m_Uaz1Inst2Motion.InitMotionManager(m_pUaz1Inst2, kForwardZ, -maxDist, maxDist, 7.0f);

	// Init Render
	SSceneLightDeferredRenderConfig renderConfig;
	renderConfig.pScene = &m_Scene;
	renderConfig.ActiveCameraIndex = 0;
	renderConfig.MaxSceneLightCount = 30;
	renderConfig.AmbientLight = { 0.8f, 0.8f, 1.0f, 1.0f };
	renderConfig.GBufferWidth = GraphicsCore::GetWindowWidth();
	renderConfig.GBufferHeight = GraphicsCore::GetWindowHeight();
	m_SceneRender.InitRender(renderConfig);

	SShadowMapConfig ShadowMapConfig;
	ShadowMapConfig.LightIndexForCastShadow = 0;
	ShadowMapConfig.MaxModelInstanceCount = 100;
	ShadowMapConfig.OpenBackFaceCulling = false;
	ShadowMapConfig.pScene = &m_Scene;
	ShadowMapConfig.Size = 8192;
	m_ShadowMapEffect.Init(ShadowMapConfig);
	m_ShadowMapEffect.UpdateShadowTransform({ 0.0f, 0.0f, 0.0f }, 120.0f);

	SSkyEffectConfig SkyEffectConfig;
	SkyEffectConfig.CubeMapName = L"sky_cube";
	SkyEffectConfig.RenderTargetFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	SkyEffectConfig.pMainCamera = m_Scene.GetCamera(0);
	m_SkyEffect.Init(SkyEffectConfig);
}

void CRayTracingAO::__InitializeRasterizerResources()
{
	SGBufferConfig GBufferConfig;
	GBufferConfig.OpenBackFaceCulling = false;
	GBufferConfig.SceneRenderConfig.ActiveCameraIndex = 0;
	GBufferConfig.SceneRenderConfig.EnableNormalMap = true;
	GBufferConfig.SceneRenderConfig.EnableNormalMapCompression = true;
	GBufferConfig.SceneRenderConfig.MaxModelInstanceCount = 100;
	GBufferConfig.SceneRenderConfig.pScene = &m_Scene;
	GBufferConfig.Width = GraphicsCore::GetWindowWidth();
	GBufferConfig.Height = GraphicsCore::GetWindowHeight();
	GBufferConfig.DepthStencilBufferFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	m_GBufferEffect.Init(GBufferConfig);

	m_AccumEffect.Init(DXGI_FORMAT_R32_FLOAT, GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), false);
	m_ImageEffect.Init(1.0f);
	m_ImageEffectDebugAccum.Init(0.25f, 0.02f, 0.65f);
	m_ImageEffectDebugAoDiff.Init(0.25f, 0.02f, 0.35f);
	m_ImageEffectDebugDynamicMark.Init(0.25f, 0.02f, 0.05f);
	m_FxaaEffect.Init(DXGI_FORMAT_R16G16B16A16_FLOAT);

	SMarkDynamicConfig MarkDynamicConfig;
	MarkDynamicConfig.WinWidth = GraphicsCore::GetWindowWidth();
	MarkDynamicConfig.WinHeight = GraphicsCore::GetWindowHeight();
	MarkDynamicConfig.DepthStencilBufferFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	MarkDynamicConfig.pScene = &m_Scene;
	MarkDynamicConfig.ActiveCameraIndex = 0;
	MarkDynamicConfig.EnableGUI = false;
	m_MarkDynamic.Init(MarkDynamicConfig);

	m_BilateralBlurEffect.Init(DXGI_FORMAT_R32_FLOAT, GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight());

	m_AtrousWaveletTransfromBlurEffect.Init(DXGI_FORMAT_R32_FLOAT, GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight());

	m_RootStartIndex = m_SceneRender.GetRootParameterCount();
	m_SampleStartIndex = m_SceneRender.GetStaticSamplerCount();
	m_RootSignature.Reset(m_SceneRender.GetRootParameterCount() + 3, m_SceneRender.GetStaticSamplerCount() + 2);
	m_SceneRender.ConfigurateRootSignature(m_RootSignature);
	m_RootSignature.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerLinearClampDesc);
	m_RootSignature.InitStaticSampler(1, 0, GraphicsCommon::Sampler::SamplerShadowDesc);
	m_RootSignature[m_RootStartIndex].InitAsConstants(0, 2, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignature[m_RootStartIndex + 1].InitAsConstantBuffer(1, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignature[m_RootStartIndex + 2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignature.Finalize(L"ModelViewer", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	m_PSO.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosTexNormTanBitan.size(), GraphicsCommon::InputLayout::PosTexNormTanBitan.data());
	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetVertexShader(g_pModelViewerVS, sizeof(g_pModelViewerVS));
	m_PSO.SetPixelShader(g_pModelViewerPS, sizeof(g_pModelViewerPS));
	auto RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
	m_PSO.SetRasterizerState(RasterizerState);
	m_PSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PSO.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
	m_PSO.SetSampleMask(UINT_MAX);
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PSO.SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_D32_FLOAT);
	m_PSO.Finalize();
}

void CRayTracingAO::__InitializeRayTracingResources()
{
	_ASSERTE(RaytracingUtility::IsDxrSupported(GraphicsCore::GetD3DDevice()));

	m_Scene.InitRaytracingScene();
	m_Scene.AddTopLevelAccelerationStructure(L"RtSceneTLA", true, 1);
	m_SceneRenderHelper.InitRender(&m_Scene, 0, 0);

	m_CalcAoDiffEffect.Init(&m_Scene, 0, GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), DXGI_FORMAT_R32_FLOAT, false);
	
	// Build raytracing root signature.
	m_UserRootParmStartIndex = m_SceneRenderHelper.GetRootParameterCount();
	m_RayTracingRootSignature.Reset(m_UserRootParmStartIndex + 3, m_SceneRenderHelper.GetStaticSamplerCount());
	m_SceneRenderHelper.ConfigurateGlobalRootSignature(m_RayTracingRootSignature);
	m_RayTracingRootSignature[m_UserRootParmStartIndex + 0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_ALL);// (b0)RayGenCB
	m_RayTracingRootSignature[m_UserRootParmStartIndex + 1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2, 0, D3D12_SHADER_VISIBILITY_ALL);// (t0)g_Ids, (t1)g_AoDiff
	m_RayTracingRootSignature[m_UserRootParmStartIndex + 2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 4, 0);// (u0)g_Output, (u1)g_PreSampleCache , (u2)g_HaltonIndex, (u3)g_AccumMap
	m_RayTracingRootSignature.Finalize(L"RayTracingAO");

	// Bulid raytracing local root signature.
	m_SceneRenderHelper.BuildHitProgLocalRootSignature(m_RtLocalRootSignature);

	// Build raytracing state object.
	m_RayTracingStateObject.ResetStateObject();
	// ---Shader Program---
	m_RayTracingStateObject.SetShaderProgram(sizeof(g_pAoTracing), g_pAoTracing, { L"AoRayGen" , L"AoClosestHit" , L"AoMiss", L"AoAnyHit" });
	// ---Shader Config---
	RaytracingUtility::SShaderConfigDesc ShaderConfigDesc;
	ShaderConfigDesc.AttributeSize = sizeof(XMFLOAT2);
	ShaderConfigDesc.PayloadSize = (sizeof(float)*2);
	m_RayTracingStateObject.SetShaderConfig(ShaderConfigDesc, { L"AoRayGen" , L"AoClosestHit" , L"AoMiss", L"AoAnyHit" });
	// ---Hit Group---
	RaytracingUtility::SHitGroupDesc AoHitGroup;
	AoHitGroup.HitGroupName = L"AoHitGroup";
	AoHitGroup.ClosestHitShaderName = L"AoClosestHit";
	AoHitGroup.AnyHitShaderName = L"AoAnyHit";
	m_RayTracingStateObject.SetHitGroup(AoHitGroup);
	// ---Global Root Signature---
	m_RayTracingStateObject.SetGlobalRootSignature(m_RayTracingRootSignature);
	// ---Local Root Signature---
	m_RayTracingStateObject.SetLocalRootSignature(m_RtLocalRootSignature, { L"AoHitGroup" });
	// ---Pipeline Config---
	m_RayTracingStateObject.SetMaxRecursionDepth(1);
	m_RayTracingStateObject.FinalizeStateObject();

	// Build raytracing shader tables.
	RaytracingUtility::CRaytracingShaderTable RayGenShaderTable;
	RayGenShaderTable.ResetShaderTable(1);
	RayGenShaderTable[0].ResetShaderRecord(m_RayTracingStateObject.QueryShaderIdentifier(L"AoRayGen"));
	RayGenShaderTable[0].FinalizeShaderRecord();
	RayGenShaderTable.FinalizeShaderTable();
	RaytracingUtility::CRaytracingShaderTable MissShaderTable;
	MissShaderTable.ResetShaderTable(1);
	MissShaderTable[0].ResetShaderRecord(m_RayTracingStateObject.QueryShaderIdentifier(L"AoMiss"));
	MissShaderTable[0].FinalizeShaderRecord();
	MissShaderTable.FinalizeShaderTable();
	RaytracingUtility::CRaytracingShaderTable HitGroupShaderTable;
	m_SceneRenderHelper.BuildMeshHitShaderTable(0, &m_RayTracingStateObject, { L"AoHitGroup" }, HitGroupShaderTable);

	// Bulid dispatch rays descriptor.
	m_DispatchRaysDesc.InitDispatchRaysDesc(GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), &RayGenShaderTable, &MissShaderTable, &HitGroupShaderTable);
}

void CRayTracingAO::__InitRenderBuffers()
{
	m_SkyColorBuffer.Create2D(L"SkyColorBuffer", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_OutputBuffer.Create2D(L"Output-Buffer", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), 1, DXGI_FORMAT_R16_FLOAT);
	m_HaltonIndexBuffer.Create2D(L"Halton-Index-Buffer", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), 1, DXGI_FORMAT_R16_UINT);
	m_PreSampleCacheBuffer.Create2D(L"Pre-Sample-Cache-Buffer", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), 1, DXGI_FORMAT_R32G32_FLOAT);
	m_BlurOutputBuffer.Create2D(L"Blur-Output-Buffer", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), 1, DXGI_FORMAT_R32_FLOAT);
	m_ColorBuffer.Create2D(L"Color-Buffer", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_FinalOutputBuffer.Create2D(L"Final-Output-Buffer", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_DepthBuffer.Create2D(L"GBuffer-Depth1", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), DXGI_FORMAT_D32_FLOAT);
	m_DepthBuffer.SetClearDepth(1.0f);
}

void CRayTracingAO::__SetCameraParamter(const SCameraParamter& paramter)
{
	m_pCamera->SetLookAt(paramter.Pos, paramter.Target, paramter.Up);
}

void CRayTracingAO::__UpdateGUI()
{
	static string aoStr;
	static string aoDiffStr;

	//ImGui::SetNextWindowSize(ImVec2(500, 220));
	ImGui::SetNextWindowBgAlpha(0.75f);
	ImGui::Begin("Raytracig AO States"); 
	ImGui::SetWindowFontScale(2.5); 
	ImGui::Text("FPS:%.1f", GraphicsCore::GetFps());
	ImGui::Text("Time:%.6f", GraphicsCore::GetTimer().GetTotalSeconds());
	aoStr = m_EnableAo ? "On" : "Off";
	ImGui::Text("Raytracig AO:%s", aoStr.data());
	if (m_EnableAo)
	{
		aoDiffStr = m_CalcAoDiffEffect.GetOpenState() ? "On" : "Off";
		ImGui::Text("AO Change Rate:%s", aoDiffStr.data());
	}

	/*ImGui::SliderFloat("AoRadius", &m_CBRayGen.AORadius, 0.0f, 2.0f);
	ImGui::SliderFloat("MinT", &m_CBRayGen.MinT, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("SampleAdaptiveFactor", &m_CBRayGen.SampleAdaptiveFactor, 0.01f, 1.0f, "%.3f");
	ImGui::SliderFloat("AoDiffAdaptiveFactor", &m_CBRayGen.AoDiffAdaptiveFactor, 0.01f, 1.0f, "%.3f");
	ImGui::SliderInt("HighSampleNum", (int*)(&m_CBRayGen.HighSampleNum), 1, 64);
	ImGui::SliderInt("MediumSampleNum", (int*)(&m_CBRayGen.MediumSampleNum), 1, 64);
	ImGui::SliderInt("LowSampleNum", (int*)(&m_CBRayGen.LowSampleNum), 1, 64);
	ImGui::SliderInt("MaxAccumCount", (int*)(&m_CBRayGen.MaxAccumCount), 1, 500);
	float GrassPatchBoundingObjectScaleFactor = m_Scene.FetchGrassManager().GetGrassPatchBoundingObjectScaleFactor();
	if (ImGui::SliderFloat("BoundingObjectScaleFactor", &GrassPatchBoundingObjectScaleFactor, 0.01f, 10.0f, "%.2f"))
	{
		m_Scene.FetchGrassManager().SetGrarssPatchBoundingObjectScaleFactor(GrassPatchBoundingObjectScaleFactor);
	}*/
	ImGui::End();
}