#include "PartialDerivativesEffect.h"

#include "CompiledShaders/CalculatePartialDerivativesViaCentralDifferencesCS.h"

CPartialDerivativesEffect::CPartialDerivativesEffect()
{
}

CPartialDerivativesEffect::~CPartialDerivativesEffect()
{
}

void CPartialDerivativesEffect::Init()
{
	__BuildRootSignature();
	__BuildPSO();
}

void CPartialDerivativesEffect::CalcPartialDerivatives(CColorBuffer* pInputTex, CColorBuffer* pOutputPartialDerivatives, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pInputTex);
	_ASSERTE(pOutputPartialDerivatives);
	_ASSERTE(pComputeCommandList);

	pComputeCommandList->TransitionResource(*pInputTex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pOutputPartialDerivatives, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pComputeCommandList->ClearUAV(*pOutputPartialDerivatives);
	pComputeCommandList->SetPipelineState(m_Pso);
	pComputeCommandList->SetRootSignature(m_RootSignature);
	pComputeCommandList->SetDynamicDescriptor(0, 0, pInputTex->GetSRV());
	pComputeCommandList->SetDynamicDescriptor(1, 0, pOutputPartialDerivatives->GetUAV());

	UINT texWidth = pInputTex->GetWidth();
	UINT texHeight = pInputTex->GetHeight();
	XMUINT2 groupSize(CeilDivide(texWidth, 8), CeilDivide(texHeight, 8));
	pComputeCommandList->Dispatch(groupSize.x, groupSize.y, 1);
}

void CPartialDerivativesEffect::__BuildRootSignature()
{
	m_RootSignature.Reset(2, 0);
	m_RootSignature[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0);
	m_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
	m_RootSignature.Finalize(L"PartialDerivativesEffectSignature", D3D12_ROOT_SIGNATURE_FLAG_NONE);
}

void CPartialDerivativesEffect::__BuildPSO()
{
	m_Pso.SetRootSignature(m_RootSignature);
	m_Pso.SetComputeShader(g_pCalculatePartialDerivativesViaCentralDifferencesCS, sizeof(g_pCalculatePartialDerivativesViaCentralDifferencesCS));
	m_Pso.Finalize();
}
