#include "BlurEffect.h"
#include "../Core/GraphicsCommon.h"

#include "CompiledShaders/BlurEffectRed3x3CS.h"
#include "CompiledShaders/BlurEffectRed5x5CS.h"
#include "CompiledShaders/BlurEffectRed7x7CS.h"
#include "CompiledShaders/BlurEffectRed9x9CS.h"
#include "CompiledShaders/BlurEffectRed11x11CS.h"
#include "CompiledShaders/BlurEffectRed13x13CS.h"
#include "CompiledShaders/BlurEffectRed15x15CS.h"
#include "CompiledShaders/BlurEffectRgba3x3CS.h"
#include "CompiledShaders/BlurEffectRgba5x5CS.h"
#include "CompiledShaders/BlurEffectRgba7x7CS.h"
#include "CompiledShaders/BlurEffectRgba9x9CS.h"
#include "CompiledShaders/BlurEffectRgba11x11CS.h"
#include "CompiledShaders/BlurEffectRgba13x13CS.h"
#include "CompiledShaders/BlurEffectRgba15x15CS.h"

CBlurEffect::CBlurEffect()
{
}

CBlurEffect::~CBlurEffect()
{
}

void CBlurEffect::Init(DXGI_FORMAT InputFormat, UINT InputWidth, UINT InputHeight)
{
	m_TemporalBuffer.Create2D(L"BlurTemporalBuffer", InputWidth, InputHeight, 1, InputFormat);

	m_BlurImageSize = XMUINT2(InputWidth, InputHeight);

	SBlurParamCB BlurParam;
	BlurParam.InvImageSize = XMFLOAT2(1.0f / InputWidth, 1.0f / InputHeight);
	m_BlurParamCB.Create(L"CBBlur", 1, sizeof(BlurParam), &BlurParam);

	__BuildRootSignature();
	__InitBlurPsShaders();
	__BuildPSO(InputFormat);
}

void CBlurEffect::BlurImage(UINT BlurCount, UINT BlurRadius, CColorBuffer* pLinearDepthBuffer, CColorBuffer* pInputBuffer, CColorBuffer* pOutputBuffer, CComputeCommandList* pCommandList)
{
	_ASSERTE(pLinearDepthBuffer && pInputBuffer && pOutputBuffer && pCommandList);

	if (BlurCount == 0)
	{
		pCommandList->TransitionResource(*pInputBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pCommandList->TransitionResource(*pOutputBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
		pCommandList->CopySubresource(*pOutputBuffer, 0, *pInputBuffer, 0);
		return;
	}

	if (BlurRadius != m_BlurRadius)
	{
		m_BlurRadius = BlurRadius;
		auto PsShader = __GetBlurPsShaderCode(m_BlurChannelType, m_BlurRadius);
		_ASSERTE(PsShader.first);
		m_PSO.SetComputeShader(PsShader.first, PsShader.second);
		m_PSO.Finalize();
	}

	pCommandList->TransitionResource(*pLinearDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);

	pCommandList->SetPipelineState(m_PSO);
	pCommandList->SetRootSignature(m_RootSignature);
	pCommandList->SetConstantBuffer(0, m_BlurParamCB.GetGpuVirtualAddress());
	pCommandList->SetDynamicDescriptor(2, 0, pLinearDepthBuffer->GetSRV());

	__RenderSingleDirectionFiltering(true, BlurRadius, pInputBuffer, &m_TemporalBuffer, pCommandList);
	__RenderSingleDirectionFiltering(false, BlurRadius, &m_TemporalBuffer, pOutputBuffer, pCommandList);

	for (UINT i = 1; i < BlurCount; i++)
	{
		__RenderSingleDirectionFiltering(true, BlurRadius, pOutputBuffer, &m_TemporalBuffer, pCommandList);
		__RenderSingleDirectionFiltering(false, BlurRadius, &m_TemporalBuffer, pOutputBuffer, pCommandList);
	}
}

void CBlurEffect::__BuildRootSignature()
{
	m_RootSignature.Reset(5, 2);
	m_RootSignature.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerPointClampDesc);
	m_RootSignature.InitStaticSampler(1, 0, GraphicsCommon::Sampler::SamplerLinearClampDesc);
	m_RootSignature[0].InitAsConstantBuffer(0, 0);
	m_RootSignature[1].InitAsConstants(1, 1, 0);
	m_RootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0);
	m_RootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
	m_RootSignature[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
	m_RootSignature.Finalize(L"BlurEffectRootSignature", D3D12_ROOT_SIGNATURE_FLAG_NONE);
}

void CBlurEffect::__InitBlurPsShaders()
{
	m_BlurPsShaders[EBlurChannelType::kRed][0].first = g_pBlurEffectRed3x3CS; 
	m_BlurPsShaders[EBlurChannelType::kRed][0].second = sizeof(g_pBlurEffectRed3x3CS);
	m_BlurPsShaders[EBlurChannelType::kRed][1].first = g_pBlurEffectRed5x5CS;
	m_BlurPsShaders[EBlurChannelType::kRed][1].second = sizeof(g_pBlurEffectRed5x5CS);
	m_BlurPsShaders[EBlurChannelType::kRed][2].first = g_pBlurEffectRed7x7CS;
	m_BlurPsShaders[EBlurChannelType::kRed][2].second = sizeof(g_pBlurEffectRed7x7CS);
	m_BlurPsShaders[EBlurChannelType::kRed][3].first = g_pBlurEffectRed9x9CS;
	m_BlurPsShaders[EBlurChannelType::kRed][3].second = sizeof(g_pBlurEffectRed9x9CS);
	m_BlurPsShaders[EBlurChannelType::kRed][4].first = g_pBlurEffectRed11x11CS;
	m_BlurPsShaders[EBlurChannelType::kRed][4].second = sizeof(g_pBlurEffectRed11x11CS);
	m_BlurPsShaders[EBlurChannelType::kRed][5].first = g_pBlurEffectRed13x13CS;
	m_BlurPsShaders[EBlurChannelType::kRed][5].second = sizeof(g_pBlurEffectRed13x13CS);
	m_BlurPsShaders[EBlurChannelType::kRed][6].first = g_pBlurEffectRed15x15CS;
	m_BlurPsShaders[EBlurChannelType::kRed][6].second = sizeof(g_pBlurEffectRed15x15CS);

	m_BlurPsShaders[EBlurChannelType::kRgba][0].first = g_pBlurEffectRgba3x3CS;
	m_BlurPsShaders[EBlurChannelType::kRgba][0].second = sizeof(g_pBlurEffectRgba3x3CS);
	m_BlurPsShaders[EBlurChannelType::kRgba][1].first = g_pBlurEffectRgba5x5CS;
	m_BlurPsShaders[EBlurChannelType::kRgba][1].second = sizeof(g_pBlurEffectRgba5x5CS);
	m_BlurPsShaders[EBlurChannelType::kRgba][2].first = g_pBlurEffectRgba7x7CS;
	m_BlurPsShaders[EBlurChannelType::kRgba][2].second = sizeof(g_pBlurEffectRgba7x7CS);
	m_BlurPsShaders[EBlurChannelType::kRgba][3].first = g_pBlurEffectRgba9x9CS;
	m_BlurPsShaders[EBlurChannelType::kRgba][3].second = sizeof(g_pBlurEffectRgba9x9CS);
	m_BlurPsShaders[EBlurChannelType::kRgba][4].first = g_pBlurEffectRgba11x11CS;
	m_BlurPsShaders[EBlurChannelType::kRgba][4].second = sizeof(g_pBlurEffectRgba11x11CS);
	m_BlurPsShaders[EBlurChannelType::kRgba][5].first = g_pBlurEffectRgba13x13CS;
	m_BlurPsShaders[EBlurChannelType::kRgba][5].second = sizeof(g_pBlurEffectRgba13x13CS);
	m_BlurPsShaders[EBlurChannelType::kRgba][6].first = g_pBlurEffectRgba15x15CS;
	m_BlurPsShaders[EBlurChannelType::kRgba][6].second = sizeof(g_pBlurEffectRgba15x15CS);
}

void CBlurEffect::__BuildPSO(DXGI_FORMAT InputFormat)
{
	m_BlurChannelType = __GetBlurChannelType(InputFormat);
	auto PsShader = __GetBlurPsShaderCode(m_BlurChannelType, m_BlurRadius);
	_ASSERTE(PsShader.first);
	
	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetComputeShader(PsShader.first, PsShader.second);
	m_PSO.Finalize();
}

void CBlurEffect::__RenderSingleDirectionFiltering(bool OpenHorizontalBlur, UINT BlurRadius, CColorBuffer* pInputBuffer, CColorBuffer* pOutputBuffer, CComputeCommandList* pCommandList)
{
	_ASSERTE(pInputBuffer && pOutputBuffer && pCommandList);

	pCommandList->TransitionResource(*pInputBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*pOutputBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pCommandList->ClearUAV(*pOutputBuffer);
	pCommandList->SetConstant(1, OpenHorizontalBlur, 0);
	pCommandList->SetDynamicDescriptor(3, 0, pInputBuffer->GetSRV());
	pCommandList->SetDynamicDescriptor(4, 0, pOutputBuffer->GetUAV());
	XMUINT2 groupSize(CeilDivide(m_BlurImageSize.x, 8), CeilDivide(m_BlurImageSize.y, 8));
	pCommandList->Dispatch(groupSize.x, groupSize.y, 1);
}

CBlurEffect::EBlurChannelType CBlurEffect::__GetBlurChannelType(DXGI_FORMAT InputFormat)
{
	EBlurChannelType BlurChannelType;
	switch (InputFormat)
	{
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
		BlurChannelType = EBlurChannelType::kRgba;
		break;

	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
		BlurChannelType = EBlurChannelType::kRed;
		break;

	default:_ASSERT(false);
	}

	return BlurChannelType;
}

const std::pair<const BYTE*, size_t>& CBlurEffect::__GetBlurPsShaderCode(EBlurChannelType BlurChannelType, UINT BlurRadius)
{
	_ASSERTE(BlurChannelType != EBlurChannelType::nChannelType);
	_ASSERTE(BlurRadius > 0 && BlurRadius <= MAX_BLUR_RADIUS);
	return m_BlurPsShaders[BlurChannelType][BlurRadius - 1];
}
