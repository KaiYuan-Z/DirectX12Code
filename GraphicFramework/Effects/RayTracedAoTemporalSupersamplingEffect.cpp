#include "../Core/GraphicsCommon.h"
#include "RayTracedAoTemporalSupersamplingEffect.h"

#include "CompiledShaders/RayTracedAoTemporalSupersamplingEffectReverseReprojectCS.h"
#include "CompiledShaders/RayTracedAoTemporalSupersamplingEffectBlendWithCurrentFrameCS.h"
#include "CompiledShaders/RayTracedAoTemporalSupersamplingEffectMixBluredAoCS.h"
#include "RayTracedAoTemporalSupersamplingEffect.h"


CRayTracedAoTemporalSupersamplingEffect::CRayTracedAoTemporalSupersamplingEffect()
{
}

CRayTracedAoTemporalSupersamplingEffect::~CRayTracedAoTemporalSupersamplingEffect()
{
}

void CRayTracedAoTemporalSupersamplingEffect::Init(DXGI_FORMAT outPutFrameFormat, UINT outPutFrameWidth, UINT outPutFrameHeight, bool enableGUI)
{
	_ASSERTE(outPutFrameWidth > 0 && outPutFrameHeight > 0);
	_ASSERTE(outPutFrameFormat == DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT || outPutFrameFormat == DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT);

	m_OutputFrameFormat = outPutFrameFormat;
	m_OutputFrameSize = XMUINT2(outPutFrameWidth, outPutFrameHeight);

	m_AccumAoCB.WinWidth = m_AccumMapCB.WinWidth = outPutFrameWidth;
	m_AccumAoCB.WinHeight = m_AccumMapCB.WinHeight = outPutFrameHeight;
	m_AccumMapCB.InvWinWidth = 1.0f / outPutFrameWidth;
	m_AccumMapCB.InvWinHeight = 1.0f / outPutFrameHeight;

	__BuildRootSignature();
	__BuildPSO();
	__BuildAccumMap();

	if (enableGUI)
	{
		GraphicsCore::RegisterGuiCallbakFunction(std::bind(&CRayTracedAoTemporalSupersamplingEffect::__UpdateGUI, this));
	}
}

void CRayTracedAoTemporalSupersamplingEffect::CalculateAccumMap(CColorBuffer* pPreLinearDepthBuffer, 
	CColorBuffer* pPreNormBuffer, 
	CColorBuffer* pCurLinearDepthBuffer, 
	CColorBuffer* pCurNormBuffer, 
	CColorBuffer* pCurDdxyBuffer,
	CColorBuffer* pCurMotionBuffer, 
	CColorBuffer* pCurIdBuffer, 
	CColorBuffer* pDynamicAreaMarkBuffer, 
	CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);
	_ASSERTE(pPreLinearDepthBuffer);
	_ASSERTE(pPreNormBuffer);
	_ASSERTE(pCurLinearDepthBuffer);
	_ASSERTE(pCurNormBuffer);
	_ASSERTE(pCurDdxyBuffer);
	_ASSERTE(pCurMotionBuffer);
	_ASSERTE(pCurIdBuffer);
	_ASSERTE(pDynamicAreaMarkBuffer);

	// Let back buffer become input.
	m_DoubleAccumFrame.SwapBuffer();
	m_DoubleAccumMap.SwapBuffer();

	pComputeCommandList->TransitionResource(*pPreLinearDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pPreNormBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pCurLinearDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pCurNormBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pCurDdxyBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pCurMotionBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pCurIdBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pDynamicAreaMarkBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);	
	pComputeCommandList->TransitionResource(m_DoubleAccumFrame.GetBackBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumMap.GetBackBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumFrame.GetFrontBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumMap.GetFrontBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

	pComputeCommandList->SetPipelineState(m_PsoAccumMap);
	pComputeCommandList->SetRootSignature(m_RootSignatureAccumMap);

	pComputeCommandList->SetDynamicConstantBufferView(0, sizeof(m_AccumMapCB), &m_AccumMapCB);

	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[10] = {
		pPreLinearDepthBuffer->GetSRV(),
		pPreNormBuffer->GetSRV(),
		pCurLinearDepthBuffer->GetSRV(),
		pCurNormBuffer->GetSRV(),
		pCurDdxyBuffer->GetSRV(),
		pCurMotionBuffer->GetSRV(),
		pCurIdBuffer->GetSRV(),
		pDynamicAreaMarkBuffer->GetSRV(),
		m_DoubleAccumFrame.GetBackBuffer().GetSRV(),
		m_DoubleAccumMap.GetBackBuffer().GetSRV(),
	};
	pComputeCommandList->SetDynamicDescriptors(1, 0, 10, Srvs);

	D3D12_CPU_DESCRIPTOR_HANDLE Uavs[2] = {
		m_DoubleAccumFrame.GetFrontBuffer().GetUAV(),
		m_DoubleAccumMap.GetFrontBuffer().GetUAV()
	};
	pComputeCommandList->SetDynamicDescriptors(2, 0, 2, Uavs);

	XMUINT2 groupSize(CeilDivide(m_OutputFrameSize.x, 8), CeilDivide(m_OutputFrameSize.y, 8));
	pComputeCommandList->Dispatch(groupSize.x, groupSize.y, 1);
}

void CRayTracedAoTemporalSupersamplingEffect::AccumulateFrame(UINT maxAccumCount, CColorBuffer* pCurFrameBuffer, CColorBuffer* pCurFrameDiff, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);
	_ASSERTE(pCurFrameBuffer);
	_ASSERTE(pCurFrameDiff);
	
	pComputeCommandList->TransitionResource(m_DoubleAccumFrame.GetFrontBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pCurFrameBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pCurFrameDiff, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumMap.GetFrontBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);	
	pComputeCommandList->TransitionResource(m_DoubleAccumFrame.GetBackBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumMap.GetBackBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

	pComputeCommandList->SetPipelineState(m_PsoAccumAo);
	pComputeCommandList->SetRootSignature(m_RootSignatureAccumAo);

	m_AccumAoCB.MaxAccumCount = maxAccumCount;
	pComputeCommandList->SetDynamicConstantBufferView(0, sizeof(m_AccumAoCB), &m_AccumAoCB);

	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[4] = {
		m_DoubleAccumFrame.GetFrontBuffer().GetSRV(),
		pCurFrameBuffer->GetSRV(),		
		pCurFrameDiff->GetSRV(),
		m_DoubleAccumMap.GetFrontBuffer().GetSRV()
	};
	pComputeCommandList->SetDynamicDescriptors(1, 0, 4, Srvs);

	D3D12_CPU_DESCRIPTOR_HANDLE Uavs[2] = {												
		m_DoubleAccumFrame.GetBackBuffer().GetUAV(),
		m_DoubleAccumMap.GetBackBuffer().GetUAV()
	};
	pComputeCommandList->SetDynamicDescriptors(2, 0, 2, Uavs);

	XMUINT2 groupSize(CeilDivide(m_OutputFrameSize.x, 8), CeilDivide(m_OutputFrameSize.y, 8));
	pComputeCommandList->Dispatch(groupSize.x, groupSize.y, 1);

	// For always get front buffer.
	m_DoubleAccumFrame.SwapBuffer();
	m_DoubleAccumMap.SwapBuffer();
}

void CRayTracedAoTemporalSupersamplingEffect::MixBluredResult(CColorBuffer* pBluredFrameBuffer, CColorBuffer* pAoDiffBuffer, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);
	_ASSERTE(pBluredFrameBuffer);

	pComputeCommandList->TransitionResource(*pBluredFrameBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pAoDiffBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(GetCurAccumFrame(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(GetCurAccumMap(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(m_MixBlurFrame, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pComputeCommandList->ClearUAV(m_MixBlurFrame);

	pComputeCommandList->SetPipelineState(m_PsoMixBluredAo);
	pComputeCommandList->SetRootSignature(m_RootSignatureMixBluredAo);

	pComputeCommandList->SetDynamicConstantBufferView(0, sizeof(m_MixBluredAoCB), &m_MixBluredAoCB);

	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[4] = {
		GetCurAccumFrame().GetSRV(),
		pBluredFrameBuffer->GetSRV(),
		pAoDiffBuffer->GetSRV(),
		GetCurAccumMap().GetSRV()
	};
	pComputeCommandList->SetDynamicDescriptors(1, 0, 4, Srvs);

	D3D12_CPU_DESCRIPTOR_HANDLE Uavs[1] = {
		m_MixBlurFrame.GetUAV()
	};
	pComputeCommandList->SetDynamicDescriptors(2, 0, 1, Uavs);

	XMUINT2 groupSize(CeilDivide(m_OutputFrameSize.x, 8), CeilDivide(m_OutputFrameSize.y, 8));
	pComputeCommandList->Dispatch(groupSize.x, groupSize.y, 1);
}

void CRayTracedAoTemporalSupersamplingEffect::ClearMaps(CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);

	pComputeCommandList->TransitionResource(m_MixBlurFrame, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumMap.GetFrontBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumMap.GetBackBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumFrame.GetFrontBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, false);
	pComputeCommandList->TransitionResource(m_DoubleAccumFrame.GetBackBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

	pComputeCommandList->ClearUAV(m_MixBlurFrame);
	pComputeCommandList->ClearUAV(m_DoubleAccumMap.GetFrontBuffer());
	pComputeCommandList->ClearUAV(m_DoubleAccumMap.GetBackBuffer());
	pComputeCommandList->ClearUAV(m_DoubleAccumFrame.GetFrontBuffer());
	pComputeCommandList->ClearUAV(m_DoubleAccumFrame.GetBackBuffer());
}

void CRayTracedAoTemporalSupersamplingEffect::__BuildRootSignature()
{
	m_RootSignatureAccumMap.Reset(3, 2);
	m_RootSignatureAccumMap.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerPointClampDesc);
	m_RootSignatureAccumMap.InitStaticSampler(1, 0, GraphicsCommon::Sampler::SamplerLinearClampDesc);
	m_RootSignatureAccumMap[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureAccumMap[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureAccumMap[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureAccumMap.Finalize(L"RayTracedAoAccumulationEffect-RootSignatureAccumMap", D3D12_ROOT_SIGNATURE_FLAG_NONE);

	m_RootSignatureAccumAo.Reset(3, 0);
	m_RootSignatureAccumAo[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureAccumAo[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureAccumAo[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureAccumAo.Finalize(L"RayTracedAoAccumulationEffect-RootSignatureAccumAo", D3D12_ROOT_SIGNATURE_FLAG_NONE);

	m_RootSignatureMixBluredAo.Reset(3, 0);
	m_RootSignatureMixBluredAo[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureMixBluredAo[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureMixBluredAo[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignatureMixBluredAo.Finalize(L"RayTracedAoAccumulationEffect-m_RootSignatureMixBluredAo", D3D12_ROOT_SIGNATURE_FLAG_NONE);
}

void CRayTracedAoTemporalSupersamplingEffect::__BuildPSO()
{
	m_PsoAccumMap.SetComputeShader(g_pRayTracedAoTemporalSupersamplingEffectReverseReprojectCS, sizeof(g_pRayTracedAoTemporalSupersamplingEffectReverseReprojectCS));
	m_PsoAccumMap.SetRootSignature(m_RootSignatureAccumMap);
	m_PsoAccumMap.Finalize();

	m_PsoAccumAo.SetComputeShader(g_pRayTracedAoTemporalSupersamplingEffectBlendWithCurrentFrameCS, sizeof(g_pRayTracedAoTemporalSupersamplingEffectBlendWithCurrentFrameCS));
	m_PsoAccumAo.SetRootSignature(m_RootSignatureAccumAo);
	m_PsoAccumAo.Finalize();

	m_PsoMixBluredAo.SetComputeShader(g_pRayTracedAoTemporalSupersamplingEffectMixBluredAoCS, sizeof(g_pRayTracedAoTemporalSupersamplingEffectMixBluredAoCS));
	m_PsoMixBluredAo.SetRootSignature(m_RootSignatureMixBluredAo);
	m_PsoMixBluredAo.Finalize();
}

void CRayTracedAoTemporalSupersamplingEffect::__BuildAccumMap()
{
	m_DoubleAccumMap.Create2D(L"AccumMap", m_OutputFrameSize.x, m_OutputFrameSize.y, 1, DXGI_FORMAT_R16G16B16A16_SINT);
	m_MixBlurFrame.Create2D(L"MixBlurFrame", m_OutputFrameSize.x, m_OutputFrameSize.y, 1, m_OutputFrameFormat);
	m_DoubleAccumFrame.Create2D(L"AccumFrame", m_OutputFrameSize.x, m_OutputFrameSize.y, 1, m_OutputFrameFormat);
}

void CRayTracedAoTemporalSupersamplingEffect::__UpdateGUI()
{
	ImGui::SetNextWindowSize(ImVec2(500, 200));
	ImGui::Begin("RayTracedAoAccumulationEffect");
	ImGui::Checkbox("ReUseDiffAo", &m_AccumAoCB.ReUseDiffAo);
	ImGui::SliderFloat("UsualAoDiffFactor", &m_AccumAoCB.AoDiffFactor1, 0.1f, 10.0f, "%.2f");
	ImGui::SliderFloat("GrassAoDiffFactor", &m_AccumAoCB.AoDiffFactor2, 0.1f, 10.0f, "%.2f");
	ImGui::SliderInt("UseBlurMaxTspp", (int*)(&m_MixBluredAoCB.UseBlurMaxTspp), 1, 100);
	ImGui::SliderFloat("BlurDecayStrength", &m_MixBluredAoCB.BlurDecayStrength, 0.0f, 10.0f, "%.2f");
	ImGui::End();
}