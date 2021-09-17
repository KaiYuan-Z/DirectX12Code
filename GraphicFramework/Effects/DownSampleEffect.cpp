#include "DownSampleEffect.h"

#include "CompiledShaders/DownSampleEffectRed2x2CS.h"
#include "CompiledShaders/DownSampleEffectRed3x3CS.h"
#include "CompiledShaders/DownSampleEffectRed4x4CS.h"
#include "CompiledShaders/DownSampleEffectRg2x2CS.h"
#include "CompiledShaders/DownSampleEffectRg3x3CS.h"
#include "CompiledShaders/DownSampleEffectRg4x4CS.h"

CDownSampleEffect::CDownSampleEffect()
{
}

CDownSampleEffect::~CDownSampleEffect()
{
}

void CDownSampleEffect::Init()
{
	__BuildRootSignature();
	__BuildPSO();
}

void CDownSampleEffect::DownSampleRed2x2(CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);
	pComputeCommandList->SetPipelineState(m_PsoRed2x2);
	__DownSample(2, pInputTex, pOutputTex, pComputeCommandList);
}

void CDownSampleEffect::DownSampleRed3x3(CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);
	pComputeCommandList->SetPipelineState(m_PsoRed3x3);
	__DownSample(3, pInputTex, pOutputTex, pComputeCommandList);
}

void CDownSampleEffect::DownSampleRed4x4(CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);
	pComputeCommandList->SetPipelineState(m_PsoRed4x4);
	__DownSample(4, pInputTex, pOutputTex, pComputeCommandList);
}

void CDownSampleEffect::DownSampleRg2x2(CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);
	pComputeCommandList->SetPipelineState(m_PsoRg2x2);
	__DownSample(2, pInputTex, pOutputTex, pComputeCommandList);
}

void CDownSampleEffect::DownSampleRg3x3(CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);
	pComputeCommandList->SetPipelineState(m_PsoRg3x3);
	__DownSample(3, pInputTex, pOutputTex, pComputeCommandList);
}

void CDownSampleEffect::DownSampleRg4x4(CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pComputeCommandList);
	pComputeCommandList->SetPipelineState(m_PsoRg4x4);
	__DownSample(4, pInputTex, pOutputTex, pComputeCommandList);
}

void CDownSampleEffect::__BuildRootSignature()
{
	m_RootSignature.Reset(3, 0);
	m_RootSignature[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0);
	m_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
	m_RootSignature[2].InitAsConstants(0, 2, 0);
	m_RootSignature.Finalize(L"DownSampleEffectSignature", D3D12_ROOT_SIGNATURE_FLAG_NONE);
}

void CDownSampleEffect::__BuildPSO()
{
	m_PsoRed2x2.SetRootSignature(m_RootSignature);
	m_PsoRed2x2.SetComputeShader(g_pDownSampleEffectRed2x2CS, sizeof(g_pDownSampleEffectRed2x2CS));
	m_PsoRed2x2.Finalize();

	m_PsoRed3x3.SetRootSignature(m_RootSignature);
	m_PsoRed3x3.SetComputeShader(g_pDownSampleEffectRed3x3CS, sizeof(g_pDownSampleEffectRed3x3CS));
	m_PsoRed3x3.Finalize();

	m_PsoRed4x4.SetRootSignature(m_RootSignature);
	m_PsoRed4x4.SetComputeShader(g_pDownSampleEffectRed4x4CS, sizeof(g_pDownSampleEffectRed4x4CS));
	m_PsoRed4x4.Finalize();

	m_PsoRg2x2.SetRootSignature(m_RootSignature);
	m_PsoRg2x2.SetComputeShader(g_pDownSampleEffectRg2x2CS, sizeof(g_pDownSampleEffectRg2x2CS));
	m_PsoRg2x2.Finalize();

	m_PsoRg3x3.SetRootSignature(m_RootSignature);
	m_PsoRg3x3.SetComputeShader(g_pDownSampleEffectRg3x3CS, sizeof(g_pDownSampleEffectRg3x3CS));
	m_PsoRg3x3.Finalize();

	m_PsoRg4x4.SetRootSignature(m_RootSignature);
	m_PsoRg4x4.SetComputeShader(g_pDownSampleEffectRg4x4CS, sizeof(g_pDownSampleEffectRg4x4CS));
	m_PsoRg4x4.Finalize();
}

void CDownSampleEffect::__DownSample(UINT Size, CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pInputTex);
	_ASSERTE(pOutputTex);

	pComputeCommandList->TransitionResource(*pInputTex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pOutputTex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pComputeCommandList->ClearUAV(*pOutputTex);

	pComputeCommandList->SetRootSignature(m_RootSignature);
	pComputeCommandList->SetDynamicDescriptor(0, 0, pInputTex->GetSRV());
	pComputeCommandList->SetDynamicDescriptor(1, 0, pOutputTex->GetUAV());

	UINT inTexWidth = pInputTex->GetWidth();
	UINT inTexHeight = pInputTex->GetHeight();

	pComputeCommandList->SetConstants(2, inTexWidth, inTexHeight);

	XMUINT2 groupSize(CeilDivide(inTexWidth / Size, 8), CeilDivide(inTexHeight / Size, 8));
	pComputeCommandList->Dispatch(groupSize.x, groupSize.y, 1);
}
