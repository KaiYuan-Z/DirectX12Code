#include "CalcAoDiffEffect.h"
#include "../Core/GraphicsCore.h"
#include "../Core/GraphicsCommon.h"

#include "CompiledShaders/CalcAoDiff.h"
#include "CompiledShaders/DownsampleAoDiffCS.h"

#define KernelSize 4

CCalcAoDiffEffect::CCalcAoDiffEffect()
{
}


CCalcAoDiffEffect::~CCalcAoDiffEffect()
{
}

void CCalcAoDiffEffect::Init(RaytracingUtility::CRaytracingScene* pRtScene, UINT activeCameraIndex, UINT aoMapWidth, UINT aoMapHeight, DXGI_FORMAT aoMapFormat, bool enableGUI)
{
	_ASSERTE(aoMapFormat == DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT || aoMapFormat == DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT);

	m_AoDiffWidth = aoMapWidth / KernelSize;
	m_AoDiffHeight = aoMapHeight / KernelSize;

	m_RawAoDiffMap.Create2D(L"RawAoDiffMap", aoMapWidth, aoMapHeight, 1, aoMapFormat);

	m_DownSampledLinearDepthBuffer.Create2D(L"DownSampledLinearDepthBuffer", m_AoDiffWidth, m_AoDiffHeight, 1, DXGI_FORMAT_R32_FLOAT);
	m_DownSampledNormBuffer.Create2D(L"DownSampledNormBuffer", m_AoDiffWidth, m_AoDiffHeight, 1, DXGI_FORMAT_R16G16_FLOAT);
	m_DownSampledDdxyBuffer.Create2D(L"DownSampledDdxyBuffer", m_AoDiffWidth, m_AoDiffHeight, 1, DXGI_FORMAT_R16G16_FLOAT);
	m_AoDiffMap.Create2D(L"AoDiffBuffer", m_AoDiffWidth, m_AoDiffHeight, 1, aoMapFormat);

	//UINT InitData[4] = {0, 0, 0, 0};
	//m_AoRayCounter.Create(L"CalcAoDiffEffect::AoRayCounter", 4, sizeof(UINT), &InitData);

	__InitGenTestRaysResources(pRtScene, activeCameraIndex);

	__InitCalcAoDiffMapResources();

	m_BlurAoDiffEffect.Init(aoMapFormat, m_AoDiffWidth, m_AoDiffHeight);
	m_BilateralBlurAoDiffEffect.Init(aoMapFormat, m_AoDiffWidth, m_AoDiffHeight, 1.0f, 0.1f, 4.0f, 0.8f);
	m_MeanAoDiffEffect.Init();
	m_DownSampleEffect.Init();

	if (enableGUI)
	{
		GraphicsCore::RegisterGuiCallbakFunction(std::bind(&CCalcAoDiffEffect::__UpdateGUI, this));
	}
}

void CCalcAoDiffEffect::Update(CCameraBasic* pCamera, float aoRadius, float aoMinT)
{
	_ASSERTE(pCamera);

	XMMATRIX preViewProj = pCamera->GetPreViewXMM() * pCamera->GetPreProjectXMM();
	m_RayGenCB.PreInvViewProj = XMMatrixInverse(nullptr, preViewProj);

	XMFLOAT3 CamPos = pCamera->GetPrePositionXMF3();
	m_RayGenCB.PreCameraPos = XMFLOAT4(CamPos.x, CamPos.y, CamPos.z, 1.0f);

	m_RayGenCB.AoRadius = aoRadius;
	m_RayGenCB.MinT = aoMinT;
}

void CCalcAoDiffEffect::CalcAoDiff(CColorBuffer* pPreHaltonIndex, 
	CColorBuffer* pPreSampleCacheBuffer, 
	CColorBuffer* pPreIdBuffer, 
	CColorBuffer* pCurMotionVecBuffer, 
	CColorBuffer* pCurAccumMapBuffer,
	CComputeCommandList* pComputeCommandList)
{
	__GenTestRays(pPreHaltonIndex,
		pPreSampleCacheBuffer,
		pPreIdBuffer,
		pCurMotionVecBuffer,
		pCurAccumMapBuffer,
		pComputeCommandList
	);

	__CalcAoDiffMap(pComputeCommandList);

	for (UINT i = 0; i < m_AoDiffBlurCount; i++)
	{
		if(i > 0) m_AoDiffMap.SwapBuffer();
		
		if (m_AoDiffBlurRadius > 4) m_AoDiffBlurRadius = 4;
		
		m_MeanAoDiffEffect.CalcMean(m_AoDiffBlurRadius * 2 + 1,
			&m_AoDiffMap.GetFrontBuffer(),
			&m_AoDiffMap.GetBackBuffer(),
			pComputeCommandList);
	}
}

void CCalcAoDiffEffect::CalcAoDiff(CColorBuffer* pPreHaltonIndex, 
	CColorBuffer* pPreSampleCacheBuffer, 
	CColorBuffer* pPreIdBuffer, 
	CColorBuffer* pCurLinearDepthBuffer,
	CColorBuffer* pCurMotionVecBuffer, 
	CColorBuffer* pCurAccumMapBuffer, 
	CComputeCommandList* pComputeCommandList)
{
	__GenTestRays(pPreHaltonIndex,
		pPreSampleCacheBuffer,
		pPreIdBuffer,
		pCurMotionVecBuffer,
		pCurAccumMapBuffer,
		pComputeCommandList
	);

	__CalcAoDiffMap(pComputeCommandList);

	m_DownSampleEffect.DownSampleRed4x4(pCurLinearDepthBuffer, &m_DownSampledLinearDepthBuffer, pComputeCommandList);

	m_BlurAoDiffEffect.BlurImage(m_AoDiffBlurCount,
		m_AoDiffBlurRadius,
		&m_DownSampledLinearDepthBuffer,
		//pCurLinearDepthBuffer,
		&m_AoDiffMap.GetFrontBuffer(),
		&m_AoDiffMap.GetBackBuffer(),
		pComputeCommandList);
}

void CCalcAoDiffEffect::CalcAoDiff(CColorBuffer* pPreHaltonIndex, 
	CColorBuffer* pPreSampleCacheBuffer, 
	CColorBuffer* pPreIdBuffer, 
	CColorBuffer* pCurLinearDepthBuffer, 
	CColorBuffer* pCurNormBuffer, 
	CColorBuffer* pDdxyBuffer,
	CColorBuffer* pCurMotionVecBuffer, 
	CColorBuffer* pCurAccumMapBuffer, 
	CComputeCommandList* pComputeCommandList)
{
	__GenTestRays(pPreHaltonIndex,
		pPreSampleCacheBuffer,
		pPreIdBuffer,
		pCurMotionVecBuffer,
		pCurAccumMapBuffer,
		pComputeCommandList
	);

	__CalcAoDiffMap(pComputeCommandList);

	m_DownSampleEffect.DownSampleRed4x4(pCurLinearDepthBuffer, &m_DownSampledLinearDepthBuffer, pComputeCommandList);
	m_DownSampleEffect.DownSampleRg4x4(pCurNormBuffer, &m_DownSampledNormBuffer, pComputeCommandList);
	m_DownSampleEffect.DownSampleRg4x4(pDdxyBuffer, &m_DownSampledDdxyBuffer, pComputeCommandList);

	m_BilateralBlurAoDiffEffect.BlurImage(m_AoDiffBlurCount,
		m_AoDiffBlurRadius,
		&m_DownSampledLinearDepthBuffer,
		&m_DownSampledNormBuffer,
		&m_DownSampledDdxyBuffer,
		&m_AoDiffMap.GetFrontBuffer(),
		&m_AoDiffMap.GetBackBuffer(),
		pComputeCommandList);
}

void CCalcAoDiffEffect::SetOpenState(bool open)
{
	m_RayGenCB.OpenAoDiffTest = open ? 1 : 0;
}

void CCalcAoDiffEffect::UseDynamicAoMark(bool open)
{
	m_RayGenCB.UseDynamicMark = open ? 1 : 0;
}

void CCalcAoDiffEffect::__InitGenTestRaysResources(RaytracingUtility::CRaytracingScene* pRtScene, UINT activeCameraIndex)
{
	_ASSERTE(pRtScene);
	
	UINT TLAIndex = pRtScene->AddTopLevelAccelerationStructure(L"CalcAoMarkEffectTLA", true, 1);
	_ASSERTE(TLAIndex != INVALID_OBJECT_INDEX);
	m_RtSceneRender.InitRender(pRtScene, TLAIndex, activeCameraIndex);

	// Build raytracing root signature.
	m_RaytracingUserRootParamStartIndex = m_RtSceneRender.GetRootParameterCount();
	m_RayTracingRootSignature.Reset(m_RtSceneRender.GetRootParameterCount() + 3, m_RtSceneRender.GetStaticSamplerCount());
	m_RtSceneRender.ConfigurateGlobalRootSignature(m_RayTracingRootSignature);
	m_RayTracingRootSignature[m_RaytracingUserRootParamStartIndex].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RayTracingRootSignature[m_RaytracingUserRootParamStartIndex + 1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RayTracingRootSignature[m_RaytracingUserRootParamStartIndex + 2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 5, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RayTracingRootSignature.Finalize(L"RayTracingAoDiffRoot");

	// Bulid raytracing local root signature.
	m_RtSceneRender.BuildHitProgLocalRootSignature(m_RaytracingLocalRootSignature);

	// Build raytracing state object.
	m_RayTracingStateObject.ResetStateObject();
	// ---Shader Program---
	m_RayTracingStateObject.SetShaderProgram(sizeof(g_pCalcAoDiff), g_pCalcAoDiff, { L"AoRayGen" , L"AoClosestHit" , L"AoMiss" });
	// ---Shader Config---
	RaytracingUtility::SShaderConfigDesc ShaderConfig;
	ShaderConfig.AttributeSize = (sizeof(float) * 2);
	ShaderConfig.PayloadSize = (sizeof(float) * 1);
	m_RayTracingStateObject.SetShaderConfig(ShaderConfig, { L"AoRayGen" , L"AoClosestHit" , L"AoMiss" });
	// ---Hit Group---
	RaytracingUtility::SHitGroupDesc AoHitGroup;
	AoHitGroup.HitGroupName = L"AoHitGroup";
	AoHitGroup.ClosestHitShaderName = L"AoClosestHit";
	m_RayTracingStateObject.SetHitGroup(AoHitGroup);
	// ---Global Root Signature---
	m_RayTracingStateObject.SetGlobalRootSignature(m_RayTracingRootSignature);
	// ---Local Root Signature---
	m_RayTracingStateObject.SetLocalRootSignature(m_RaytracingLocalRootSignature, { L"AoHitGroup" });
	// ---Pipeline Config---
	m_RayTracingStateObject.SetMaxRecursionDepth(1);
	m_RayTracingStateObject.FinalizeStateObject();

	// Bulid dispatch rays descriptor.
	// ---RayGenShaderTable---
	RaytracingUtility::CRaytracingShaderTable RayGenShaderTable;
	RayGenShaderTable.ResetShaderTable(1);
	RayGenShaderTable[0].ResetShaderRecord(m_RayTracingStateObject.QueryShaderIdentifier(L"AoRayGen"));
	RayGenShaderTable[0].FinalizeShaderRecord();
	RayGenShaderTable.FinalizeShaderTable();
	// ---MissShaderTable---
	RaytracingUtility::CRaytracingShaderTable MissShaderTable;
	MissShaderTable.ResetShaderTable(1);
	MissShaderTable[0].ResetShaderRecord(m_RayTracingStateObject.QueryShaderIdentifier(L"AoMiss"));
	MissShaderTable[0].FinalizeShaderRecord();
	MissShaderTable.FinalizeShaderTable();
	// ---HitGroupShaderTable---
	RaytracingUtility::CRaytracingShaderTable HitGroupShaderTable;
	m_RtSceneRender.BuildMeshHitShaderTable(TLAIndex, &m_RayTracingStateObject, { L"AoHitGroup" }, HitGroupShaderTable);
	// ---Init Dispatch Rays Descriptor---
	m_DispatchRaysDesc.InitDispatchRaysDesc(m_AoDiffWidth * KernelSize, m_AoDiffHeight * KernelSize, &RayGenShaderTable, &MissShaderTable, &HitGroupShaderTable);
}

void CCalcAoDiffEffect::__InitCalcAoDiffMapResources()
{
	m_RootSignatureCalcAoDiffMap.Reset(3, 0);
	m_RootSignatureCalcAoDiffMap[0].InitAsConstantBuffer(0, 0);
	m_RootSignatureCalcAoDiffMap[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0);
	m_RootSignatureCalcAoDiffMap[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
	m_RootSignatureCalcAoDiffMap.Finalize(L"CalcAoDiffMapSignature", D3D12_ROOT_SIGNATURE_FLAG_NONE);
	
	m_PsoCalcAoDiffMap.SetRootSignature(m_RootSignatureCalcAoDiffMap);
	m_PsoCalcAoDiffMap.SetComputeShader(g_pDownsampleAoDiffCS, sizeof(g_pDownsampleAoDiffCS));
	m_PsoCalcAoDiffMap.Finalize();

	m_DownsampleAoDiffCB.AoMapSize = XMINT2(m_AoDiffWidth * KernelSize, m_AoDiffHeight * KernelSize);
}

void CCalcAoDiffEffect::__GenTestRays(CColorBuffer* pPreHaltonIndex,
	CColorBuffer* pPreSampleCacheBuffer,
	CColorBuffer* pPreIdBuffer,
	CColorBuffer* pCurMotionVecBuffer,
	CColorBuffer* pCurAccumMapBuffer,
	CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pPreHaltonIndex);
	_ASSERTE(pPreSampleCacheBuffer);
	_ASSERTE(pPreIdBuffer);
	_ASSERTE(pCurMotionVecBuffer);
	_ASSERTE(pCurAccumMapBuffer);
	_ASSERTE(pComputeCommandList);

	pComputeCommandList->TransitionResource(m_RawAoDiffMap, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	//pComputeCommandList->TransitionResource(m_AoRayCounter, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	pComputeCommandList->TransitionResource(*pPreHaltonIndex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pPreSampleCacheBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pPreIdBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pCurMotionVecBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pCurAccumMapBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);

	//pComputeCommandList->ClearUAV(m_AoRayCounter);

	pComputeCommandList->SetRootSignature(m_RayTracingRootSignature);

	m_RtSceneRender.SetGlobalRootParameter(pComputeCommandList);

	pComputeCommandList->SetDynamicConstantBufferView(m_RaytracingUserRootParamStartIndex, sizeof(m_RayGenCB), &m_RayGenCB);

	D3D12_CPU_DESCRIPTOR_HANDLE Uavs[1] = {
		m_RawAoDiffMap.GetUAV()
		//,m_AoRayCounter.GetUAV()
	};
	pComputeCommandList->SetDynamicDescriptors(m_RaytracingUserRootParamStartIndex + 1, 0, 1, Uavs);

	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[5] = {
		pPreHaltonIndex->GetSRV(),
		pPreSampleCacheBuffer->GetSRV(),
		pPreIdBuffer->GetSRV(),
		pCurMotionVecBuffer->GetSRV(),
		pCurAccumMapBuffer->GetSRV()
	};
	pComputeCommandList->SetDynamicDescriptors(m_RaytracingUserRootParamStartIndex + 2, 0, 5, Srvs);

	RaytracingUtility::DispatchRays(&m_RayTracingStateObject, &m_DispatchRaysDesc, pComputeCommandList);
}

void CCalcAoDiffEffect::__CalcAoDiffMap(CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);

	pComputeCommandList->TransitionResource(m_RawAoDiffMap, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(m_AoDiffMap.GetFrontBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pComputeCommandList->ClearUAV(m_AoDiffMap.GetFrontBuffer());

	pComputeCommandList->SetPipelineState(m_PsoCalcAoDiffMap);
	pComputeCommandList->SetRootSignature(m_RootSignatureCalcAoDiffMap);

	pComputeCommandList->SetDynamicConstantBufferView(0, sizeof(m_DownsampleAoDiffCB), &m_DownsampleAoDiffCB);

	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[1] = {
		m_RawAoDiffMap.GetSRV()
	};
	pComputeCommandList->SetDynamicDescriptors(1, 0, 1, Srvs);

	D3D12_CPU_DESCRIPTOR_HANDLE Uavs[1] = {
		m_AoDiffMap.GetFrontBuffer().GetUAV()
	};
	pComputeCommandList->SetDynamicDescriptors(2, 0, 1, Uavs);

	XMUINT2 groupSize(CeilDivide(m_AoDiffWidth, 8), CeilDivide(m_AoDiffHeight, 8));
	pComputeCommandList->Dispatch(groupSize.x, groupSize.y, 1);
}

void CCalcAoDiffEffect::__UpdateGUI()
{
	static float AoDiffFactor = 1.0f;
	m_RayGenCB.AoDiffScale = (float)GraphicsCore::GetFps() * AoDiffFactor / 10.0f;

	ImGui::SetNextWindowSize(ImVec2(500, 100));
	ImGui::Begin("CalcAoDiffEffect");
	ImGui::Text("AoDiffScale:%f", m_RayGenCB.AoDiffScale);
	ImGui::SliderFloat("AoDiffFactor", &AoDiffFactor, 0.1f, 3.0f, "%.1f");
	ImGui::SliderInt("AoDiffBlurCount", (int*)(&m_AoDiffBlurCount), 1, 5); 
	ImGui::SliderInt("AoDiffBlurRadius", (int*)(&m_AoDiffBlurRadius), 1, 7); 
	ImGui::Checkbox("UseDynamicMark", (bool*)(&m_RayGenCB.UseDynamicMark));
	ImGui::Checkbox("OpenAoDiffTest", (bool*)(&m_RayGenCB.OpenAoDiffTest));
	ImGui::End();

	
}
