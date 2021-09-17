#include "DecodeNormalEffect.h"

#include "CompiledShaders/DecodeNormalCS.h"

CDecodeNormalEffect::CDecodeNormalEffect()
{
}

CDecodeNormalEffect::~CDecodeNormalEffect()
{
}

void CDecodeNormalEffect::Init()
{
	__BuildRootSignature();
	__BuildPSO();
}

void CDecodeNormalEffect::DecodeNormal(CColorBuffer* pInputFloat2Buffer, CColorBuffer* pOutputFloat4Buffer, CComputeCommandList* pComputeCommandList)
{
	_ASSERTE(pInputFloat2Buffer);
	_ASSERTE(pOutputFloat4Buffer);
	_ASSERTE(pComputeCommandList);

	_ASSERTE(pInputFloat2Buffer->GetFormat() == DXGI_FORMAT_R16G16_FLOAT 
		|| pInputFloat2Buffer->GetFormat() == DXGI_FORMAT_R32G32_FLOAT);
	_ASSERTE(pOutputFloat4Buffer->GetFormat() == DXGI_FORMAT_R16G16B16A16_FLOAT
		|| pOutputFloat4Buffer->GetFormat() == DXGI_FORMAT_R32G32B32A32_FLOAT);

	UINT inTexWidth = pInputFloat2Buffer->GetWidth();
	UINT inTexHeight = pInputFloat2Buffer->GetHeight();
	_ASSERTE(inTexWidth == pOutputFloat4Buffer->GetWidth()
		&& inTexHeight == pOutputFloat4Buffer->GetHeight());

	pComputeCommandList->TransitionResource(*pInputFloat2Buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pComputeCommandList->TransitionResource(*pOutputFloat4Buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

	pComputeCommandList->SetRootSignature(m_RootSignature);
	pComputeCommandList->SetPipelineState(m_Pso);

	pComputeCommandList->SetDynamicDescriptor(0, 0, pInputFloat2Buffer->GetSRV());
	pComputeCommandList->SetDynamicDescriptor(1, 0, pOutputFloat4Buffer->GetUAV());

	XMUINT2 groupSize(CeilDivide(inTexWidth, 8), CeilDivide(inTexHeight, 8));
	pComputeCommandList->Dispatch(groupSize.x, groupSize.y, 1);
}

void CDecodeNormalEffect::__BuildRootSignature()
{
	m_RootSignature.Reset(2, 0);
	m_RootSignature[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0);
	m_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
	m_RootSignature.Finalize(L"DecodeNormalEffectSignature", D3D12_ROOT_SIGNATURE_FLAG_NONE);
}

void CDecodeNormalEffect::__BuildPSO()
{
	m_Pso.SetRootSignature(m_RootSignature);
	m_Pso.SetComputeShader(g_pDecodeNormalCS, sizeof(g_pDecodeNormalCS));
	m_Pso.Finalize();
}
