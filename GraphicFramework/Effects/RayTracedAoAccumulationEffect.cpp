#include "../Core/GraphicsCommon.h"
#include "RayTracedAoAccumulationEffect.h"

#include "CompiledShaders/RayTracedAoAccumulationEffectAccumMapCS.h"
#include "CompiledShaders/RayTracedAoAccumulationEffectAccumAoCS.h"
#include "CompiledShaders/RayTracedAoAccumulationEffectMixBluredAoCS.h"


CRayTracedAoAccumulationEffect::CRayTracedAoAccumulationEffect()
{
}

CRayTracedAoAccumulationEffect::~CRayTracedAoAccumulationEffect()
{
}

void CRayTracedAoAccumulationEffect::Init(DXGI_FORMAT outPutFrameFormat, UINT outPutFrameWidth, UINT outPutFrameHeight)
{
	_ASSERTE(outPutFrameWidth > 0 && outPutFrameHeight > 0);
	_ASSERTE(outPutFrameFormat == DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT || outPutFrameFormat == DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT);

	m_OutputFrameFormat = outPutFrameFormat;
	m_OutputFrameSize = XMUINT2(outPutFrameWidth, outPutFrameHeight);

	m_AccumAoCB.WinWidth = m_AccumMapCB.WinWidth = outPutFrameWidth;
	m_AccumAoCB.WinHeight = m_AccumMapCB.WinHeight = outPutFrameHeight;

	__BuildRootSignature();
	__BuildPSO();
	__BuildAccumMap();

	GraphicsCore::RegisterGuiCallbakFunction(std::bind(&CRayTracedAoAccumulationEffect::__UpdateGUI, this));
}

void CRayTracedAoAccumulationEffect::CalculateAccumMap(CColorBuffer* pPreLinearDepthBuffer, 
	CColorBuffer* pCurLinearDepthBuffer, 
	CColorBuffer* pMotionBuffer, 
	CColorBuffer* pDynamicAreaMarkBuffer, 
	CColorBuffer* pIdMapBuffer, 
	CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);
	_ASSERTE(pPreLinearDepthBuffer);
	_ASSERTE(pCurLinearDepthBuffer);
	_ASSERTE(pMotionBuffer);
	_ASSERTE(pDynamicAreaMarkBuffer);
	_ASSERTE(pIdMapBuffer);

	m_DoubleAccumMap.SwapBuffer();

	pComputeCommandList->TransitionResource(*pPreLinearDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pCurLinearDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pMotionBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pDynamicAreaMarkBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pIdMapBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumMap.GetBackBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumMap.GetFrontBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pComputeCommandList->ClearUAV(m_DoubleAccumMap.GetFrontBuffer());

	pComputeCommandList->SetPipelineState(m_PsoAccumMap);
	pComputeCommandList->SetRootSignature(m_RootSignatureAccumMap);

	pComputeCommandList->SetDynamicConstantBufferView(0, sizeof(m_AccumMapCB), &m_AccumMapCB);

	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[6] = {
		pPreLinearDepthBuffer->GetSRV(),
		pCurLinearDepthBuffer->GetSRV(),
		pMotionBuffer->GetSRV(),
		pDynamicAreaMarkBuffer->GetSRV(),
		pIdMapBuffer->GetSRV(),
		m_DoubleAccumMap.GetBackBuffer().GetSRV()
	};
	pComputeCommandList->SetDynamicDescriptors(1, 0, 6, Srvs);

	D3D12_CPU_DESCRIPTOR_HANDLE Uavs[1] = {
		m_DoubleAccumMap.GetFrontBuffer().GetUAV()
	};
	pComputeCommandList->SetDynamicDescriptors(2, 0, 1, Uavs);

	XMUINT2 groupSize(CeilDivide(m_OutputFrameSize.x, 8), CeilDivide(m_OutputFrameSize.y, 8));
	pComputeCommandList->Dispatch(groupSize.x, groupSize.y, 1);
}

void CRayTracedAoAccumulationEffect::AccumulateFrame(CColorBuffer* pCurFrameBuffer, 
	CColorBuffer* pMotionBuffer, 
	CColorBuffer* pLocalMeanVarianceBuffer, 
	CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);
	_ASSERTE(pCurFrameBuffer);
	_ASSERTE(pMotionBuffer);
	_ASSERTE(pLocalMeanVarianceBuffer);

	m_DoubleAccumFrame.SwapBuffer();
	m_DoubleAccumSquaredMean.SwapBuffer();
	
	pComputeCommandList->TransitionResource(m_DoubleAccumFrame.GetBackBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumSquaredMean.GetBackBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pCurFrameBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pMotionBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pLocalMeanVarianceBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumMap.GetFrontBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);	
	pComputeCommandList->TransitionResource(m_DoubleAccumFrame.GetFrontBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumSquaredMean.GetFrontBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	pComputeCommandList->TransitionResource(m_AccumVariance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumMap.GetBackBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pComputeCommandList->ClearUAV(m_DoubleAccumFrame.GetFrontBuffer());
	pComputeCommandList->ClearUAV(m_DoubleAccumSquaredMean.GetFrontBuffer());
	pComputeCommandList->ClearUAV(m_AccumVariance);
	pComputeCommandList->ClearUAV(m_DoubleAccumMap.GetBackBuffer());

	pComputeCommandList->SetPipelineState(m_PsoAccumAo);
	pComputeCommandList->SetRootSignature(m_RootSignatureAccumAo);

	pComputeCommandList->SetDynamicConstantBufferView(0, sizeof(m_AccumAoCB), &m_AccumAoCB);

	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[6] = {
		m_DoubleAccumFrame.GetBackBuffer().GetSRV(),
		m_DoubleAccumSquaredMean.GetBackBuffer().GetSRV(),
		pCurFrameBuffer->GetSRV(),						
		pMotionBuffer->GetSRV(),						
		pLocalMeanVarianceBuffer->GetSRV(),
		m_DoubleAccumMap.GetFrontBuffer().GetSRV()
	};
	pComputeCommandList->SetDynamicDescriptors(1, 0, 6, Srvs);

	D3D12_CPU_DESCRIPTOR_HANDLE Uavs[4] = {												
		m_DoubleAccumFrame.GetFrontBuffer().GetUAV(),	
		m_DoubleAccumSquaredMean.GetFrontBuffer().GetUAV(),
		m_AccumVariance.GetUAV(),	
		m_DoubleAccumMap.GetBackBuffer().GetUAV(),
	};
	pComputeCommandList->SetDynamicDescriptors(2, 0, 4, Uavs);

	XMUINT2 groupSize(CeilDivide(m_OutputFrameSize.x, 8), CeilDivide(m_OutputFrameSize.y, 8));
	pComputeCommandList->Dispatch(groupSize.x, groupSize.y, 1);

	m_DoubleAccumMap.SwapBuffer();
}

void CRayTracedAoAccumulationEffect::MixBluredResult(CColorBuffer* pBluredFrameBuffer, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);
	_ASSERTE(pBluredFrameBuffer);

	pComputeCommandList->TransitionResource(*pBluredFrameBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(GetCurAccumFrame(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(GetCurAccumMap(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(m_MixBlurFrame, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pComputeCommandList->ClearUAV(m_MixBlurFrame);

	pComputeCommandList->SetPipelineState(m_PsoMixBluredAo);
	pComputeCommandList->SetRootSignature(m_RootSignatureMixBluredAo);

	pComputeCommandList->SetDynamicConstantBufferView(0, sizeof(m_MixBluredAoCB), &m_MixBluredAoCB);

	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[3] = {
		GetCurAccumFrame().GetSRV(),
		pBluredFrameBuffer->GetSRV(),
		GetCurAccumMap().GetSRV()
	};
	pComputeCommandList->SetDynamicDescriptors(1, 0, 3, Srvs);

	D3D12_CPU_DESCRIPTOR_HANDLE Uavs[1] = {
		m_MixBlurFrame.GetUAV()
	};
	pComputeCommandList->SetDynamicDescriptors(2, 0, 1, Uavs);

	XMUINT2 groupSize(CeilDivide(m_OutputFrameSize.x, 8), CeilDivide(m_OutputFrameSize.y, 8));
	pComputeCommandList->Dispatch(groupSize.x, groupSize.y, 1);
}

void CRayTracedAoAccumulationEffect::__BuildRootSignature()
{
	m_RootSignatureAccumMap.Reset(3, 2);
	m_RootSignatureAccumMap.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerPointBorderDesc);
	m_RootSignatureAccumMap.InitStaticSampler(1, 0, GraphicsCommon::Sampler::SamplerLinearBorderDesc);
	m_RootSignatureAccumMap[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureAccumMap[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureAccumMap[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureAccumMap.Finalize(L"RayTracedAoAccumulationEffect-RootSignatureAccumMap", D3D12_ROOT_SIGNATURE_FLAG_NONE);

	m_RootSignatureAccumAo.Reset(3, 2);
	m_RootSignatureAccumAo.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerPointClampDesc);
	m_RootSignatureAccumAo.InitStaticSampler(1, 0, GraphicsCommon::Sampler::SamplerLinearClampDesc);
	m_RootSignatureAccumAo[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureAccumAo[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureAccumAo[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 4, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureAccumAo.Finalize(L"RayTracedAoAccumulationEffect-RootSignatureAccumAo", D3D12_ROOT_SIGNATURE_FLAG_NONE);

	m_RootSignatureMixBluredAo.Reset(3, 0);
	m_RootSignatureMixBluredAo[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureMixBluredAo[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureMixBluredAo[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureMixBluredAo.Finalize(L"RayTracedAoAccumulationEffect-m_RootSignatureMixBluredAo", D3D12_ROOT_SIGNATURE_FLAG_NONE);
}

void CRayTracedAoAccumulationEffect::__BuildPSO()
{
	m_PsoAccumMap.SetComputeShader(g_pRayTracedAoAccumulationEffectAccumMapCS, sizeof(g_pRayTracedAoAccumulationEffectAccumMapCS));
	m_PsoAccumMap.SetRootSignature(m_RootSignatureAccumMap);
	m_PsoAccumMap.Finalize();

	m_PsoAccumAo.SetComputeShader(g_pRayTracedAoAccumulationEffectAccumAoCS, sizeof(g_pRayTracedAoAccumulationEffectAccumAoCS));
	m_PsoAccumAo.SetRootSignature(m_RootSignatureAccumAo);
	m_PsoAccumAo.Finalize();

	m_PsoMixBluredAo.SetComputeShader(g_pRayTracedAoAccumulationEffectMixBluredAoCS, sizeof(g_pRayTracedAoAccumulationEffectMixBluredAoCS));
	m_PsoMixBluredAo.SetRootSignature(m_RootSignatureMixBluredAo);
	m_PsoMixBluredAo.Finalize();
}

void CRayTracedAoAccumulationEffect::__BuildAccumMap()
{
	m_DoubleAccumMap.Create2D(L"AccumMap", m_OutputFrameSize.x, m_OutputFrameSize.y, 1, DXGI_FORMAT_R16G16B16A16_SINT);
	m_AccumVariance.Create2D(L"AccumVariance", m_OutputFrameSize.x, m_OutputFrameSize.y, 1, m_OutputFrameFormat);
	m_DoubleAccumSquaredMean.Create2D(L"AccumSquaredMean", m_OutputFrameSize.x, m_OutputFrameSize.y, 1, m_OutputFrameFormat);
	m_MixBlurFrame.Create2D(L"MixBlurFrame", m_OutputFrameSize.x, m_OutputFrameSize.y, 1, m_OutputFrameFormat);
	m_DoubleAccumFrame.Create2D(L"AccumFrame", m_OutputFrameSize.x, m_OutputFrameSize.y, 1, m_OutputFrameFormat);
}

void CRayTracedAoAccumulationEffect::__UpdateGUI()
{
	ImGui::SetNextWindowSize(ImVec2(750, 300));
	ImGui::Begin("RayTracedAoAccumulationEffect");
	ImGui::SliderInt("AccumMap_DynamicMarkMaxLife", (int*)(&m_AccumMapCB.DynamicMarkMaxLife), 1, 10);
	ImGui::SliderFloat("AccumMap_ReprojTestFactor", &m_AccumMapCB.ReprojTestFactor, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("AccumAo_StdDevGamma", &m_AccumAoCB.StdDevGamma, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("AccumAo_ClampMinStdDevTolerance", &m_AccumAoCB.ClampMinStdDevTolerance, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("AccumAo_ClampDifferenceToTsppScale", &m_AccumAoCB.ClampDifferenceToTsppScale, 0.0f, 500.0f, "%.2f");
	ImGui::SliderFloat("AccumAo_MinSmoothingFactor", &m_AccumAoCB.MinSmoothingFactor, 0.0f, 1.0f, "%.2f");
	ImGui::SliderInt("AccumAo_MinTsppToUseTemporalVariance", (int*)(&m_AccumAoCB.MinTsppToUseTemporalVariance), 1, 100);
	ImGui::SliderInt("AccumAo_MinNsppForClamp", (int*)(&m_AccumAoCB.MinNsppForClamp), 1, 100);
	ImGui::SliderInt("AccumAo_MaxAccumCount", (int*)(&m_AccumAoCB.MaxAccumCount), 1, 500);
	ImGui::SliderInt("MixBlur_UseBlurMaxTspp", (int*)(&m_MixBluredAoCB.UseBlurMaxTspp), 1, 100);
	ImGui::SliderFloat("MixBlur_BlurDecayStrength", &m_MixBluredAoCB.BlurDecayStrength, 0.0f, 10.0f, "%.2f");
	ImGui::End();

	m_AccumAoCB.DynamicMarkMaxLife = m_AccumMapCB.DynamicMarkMaxLife;
}
