#include "MeanVarianceEffect.h"

#include "CompiledShaders/CalculateMeanCS.h"
#include "CompiledShaders/CalculateVarianceCS.h"
#include "CompiledShaders/CalculateMeanVarianceCS.h"

CMeanVarianceEffect::CMeanVarianceEffect()
{
}

CMeanVarianceEffect::~CMeanVarianceEffect()
{
}

void CMeanVarianceEffect::Init()
{
	__BuildRootSignature();
	__BuildPSO();
}

void CMeanVarianceEffect::CalcMean(UINT kernelWidth, CColorBuffer* pInputTex, CColorBuffer* pOutputMean, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pInputTex);
	_ASSERTE(pOutputMean);
	_ASSERTE(pInputTex->GetWidth() == pOutputMean->GetWidth() && pInputTex->GetHeight() == pOutputMean->GetHeight());
	_ASSERTE(pComputeCommandList);

	pComputeCommandList->TransitionResource(*pInputTex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pOutputMean, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pComputeCommandList->ClearUAV(*pOutputMean);
	pComputeCommandList->SetPipelineState(m_PsoMean);
	pComputeCommandList->SetRootSignature(m_RootSignatureMean);
	pComputeCommandList->SetDynamicConstantBufferView(0, sizeof(m_MeanVarianceCB), &m_MeanVarianceCB);
	pComputeCommandList->SetDynamicDescriptor(1, 0, pInputTex->GetSRV());
	pComputeCommandList->SetDynamicDescriptor(2, 0, pOutputMean->GetUAV());

	__Dispatch(kernelWidth, pInputTex->GetWidth(), pInputTex->GetHeight(), pComputeCommandList);
}

void CMeanVarianceEffect::CalcVariance(UINT kernelWidth, CColorBuffer* pInputTex, CColorBuffer* pOutputVariance, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pInputTex);
	_ASSERTE(pOutputVariance);
	_ASSERTE(pInputTex->GetWidth() == pOutputVariance->GetWidth() && pInputTex->GetHeight() == pOutputVariance->GetHeight());
	_ASSERTE(pComputeCommandList);

	pComputeCommandList->TransitionResource(*pInputTex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pOutputVariance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pComputeCommandList->ClearUAV(*pOutputVariance);
	pComputeCommandList->SetPipelineState(m_PsoVariance);
	pComputeCommandList->SetRootSignature(m_RootSignatureVariance);
	pComputeCommandList->SetDynamicConstantBufferView(0, sizeof(m_MeanVarianceCB), &m_MeanVarianceCB);
	pComputeCommandList->SetDynamicDescriptor(1, 0, pInputTex->GetSRV());
	pComputeCommandList->SetDynamicDescriptor(2, 0, pOutputVariance->GetUAV());

	__Dispatch(kernelWidth, pInputTex->GetWidth(), pInputTex->GetHeight(), pComputeCommandList);
}

void CMeanVarianceEffect::CalcMeanVariance(UINT kernelWidth, CColorBuffer* pInputTex, CColorBuffer* pOutputMeanVariance, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pInputTex);
	_ASSERTE(pOutputMeanVariance);
	_ASSERTE(pInputTex->GetWidth() == pOutputMeanVariance->GetWidth() && pInputTex->GetHeight() == pOutputMeanVariance->GetHeight());
	_ASSERTE(pComputeCommandList);

	pComputeCommandList->TransitionResource(*pInputTex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pOutputMeanVariance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pComputeCommandList->ClearUAV(*pOutputMeanVariance);
	pComputeCommandList->SetPipelineState(m_PsoMeanVariance);
	pComputeCommandList->SetRootSignature(m_RootSignatureMeanVariance);
	pComputeCommandList->SetDynamicConstantBufferView(0, sizeof(m_MeanVarianceCB), &m_MeanVarianceCB);
	pComputeCommandList->SetDynamicDescriptor(1, 0, pInputTex->GetSRV());
	D3D12_CPU_DESCRIPTOR_HANDLE Uavs[1] = { pOutputMeanVariance->GetUAV() };
	pComputeCommandList->SetDynamicDescriptors(2, 0, 1, Uavs);

	__Dispatch(kernelWidth, pInputTex->GetWidth(), pInputTex->GetHeight(), pComputeCommandList);
}

void CMeanVarianceEffect::__BuildRootSignature()
{
	m_RootSignatureMean.Reset(3, 0);
	m_RootSignatureMean[0].InitAsConstantBuffer(0, 0);
	m_RootSignatureMean[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0);
	m_RootSignatureMean[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
	m_RootSignatureMean.Finalize(L"MeanVarianceEffectSignature1", D3D12_ROOT_SIGNATURE_FLAG_NONE);

	m_RootSignatureVariance.Reset(3, 0);
	m_RootSignatureVariance[0].InitAsConstantBuffer(0, 0);
	m_RootSignatureVariance[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0);
	m_RootSignatureVariance[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
	m_RootSignatureVariance.Finalize(L"MeanVarianceEffectSignature2", D3D12_ROOT_SIGNATURE_FLAG_NONE);

	m_RootSignatureMeanVariance.Reset(3, 0);
	m_RootSignatureMeanVariance[0].InitAsConstantBuffer(0, 0);
	m_RootSignatureMeanVariance[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0);
	m_RootSignatureMeanVariance[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
	m_RootSignatureMeanVariance.Finalize(L"MeanVarianceEffectSignature3", D3D12_ROOT_SIGNATURE_FLAG_NONE);
}

void CMeanVarianceEffect::__BuildPSO()
{
	m_PsoMean.SetRootSignature(m_RootSignatureMean);
	m_PsoMean.SetComputeShader(g_pCalculateMeanCS, sizeof(g_pCalculateMeanCS));
	m_PsoMean.Finalize();
	
	m_PsoVariance.SetRootSignature(m_RootSignatureVariance);
	m_PsoVariance.SetComputeShader(g_pCalculateVarianceCS, sizeof(g_pCalculateVarianceCS));
	m_PsoVariance.Finalize();
	
	m_PsoMeanVariance.SetRootSignature(m_RootSignatureMeanVariance);
	m_PsoMeanVariance.SetComputeShader(g_pCalculateMeanVarianceCS, sizeof(g_pCalculateMeanVarianceCS));
	m_PsoMeanVariance.Finalize();
}

void CMeanVarianceEffect::__Dispatch(UINT kernelWidth, UINT texWidth, UINT texHeight, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(kernelWidth >= 1 && kernelWidth <= 9 && kernelWidth % 2 > 0);
	_ASSERTE(pComputeCommandList);
	
	m_MeanVarianceCB.KernelWidth = kernelWidth;
	m_MeanVarianceCB.KernelRadius = kernelWidth >> 1;
	m_MeanVarianceCB.TextureDim = XMUINT2(texWidth, texHeight);

	XMUINT2 groupSize(CeilDivide(texWidth, 8), CeilDivide(texHeight, 8));
	pComputeCommandList->Dispatch(groupSize.x, groupSize.y, 1);
}
