#include "BilateralBlurEffect.h"
#include "../Core/GraphicsCommon.h"

#include "CompiledShaders/BilateralBlurEffectRed3x3CS.h"
#include "CompiledShaders/BilateralBlurEffectRed5x5CS.h"
#include "CompiledShaders/BilateralBlurEffectRed7x7CS.h"
#include "CompiledShaders/BilateralBlurEffectRed9x9CS.h"
#include "CompiledShaders/BilateralBlurEffectRed11x11CS.h"
#include "CompiledShaders/BilateralBlurEffectRed13x13CS.h"
#include "CompiledShaders/BilateralBlurEffectRed15x15CS.h"

CBilateralBlurEffect::CBilateralBlurEffect()
{
}

CBilateralBlurEffect::~CBilateralBlurEffect()
{
}

void CBilateralBlurEffect::Init(DXGI_FORMAT InputFormat, UINT InputWidth, UINT InputHeight, float DepthSigma, float DepthThreshold, float NormalSigma, float NormalThreshold)
{
	m_TemporalBuffer.Create2D(L"BlurTemporalBuffer", InputWidth, InputHeight, 1, InputFormat);

	m_BlurImageSize = XMUINT2(InputWidth, InputHeight);
	m_BlurParamCB.ImageSize = m_BlurImageSize;
	m_BlurParamCB.InvImageSize = XMFLOAT2(1.0f / InputWidth, 1.0f / InputHeight);
	m_BlurParamCB.DepthSigma = DepthSigma;
	m_BlurParamCB.DepthThreshold = DepthThreshold;
	m_BlurParamCB.NormalSigma = NormalSigma;
	m_BlurParamCB.NormalThreshold = NormalThreshold;
	__BuildRootSignature();
	__InitBlurPsShaders();
	__BuildPSO(InputFormat);
}

void CBilateralBlurEffect::BlurImage(UINT BlurCount,
	UINT BlurRadius,
	CColorBuffer* pLinearDepthBuffer,
	CColorBuffer* pNormBuffer,
	CColorBuffer* pDdxyBuffer,
	CColorBuffer* pInputBuffer,
	CColorBuffer* pOutputBuffer,
	CComputeCommandList* pCommandList)
{
	_ASSERTE(pLinearDepthBuffer);
	_ASSERTE(pNormBuffer);
	_ASSERTE(pDdxyBuffer);
	_ASSERTE(pInputBuffer);
	_ASSERTE(pOutputBuffer);
	_ASSERTE(pCommandList);

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

	pCommandList->TransitionResource(*pLinearDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*pNormBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*pDdxyBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);

	pCommandList->SetPipelineState(m_PSO);
	pCommandList->SetRootSignature(m_RootSignature);
	pCommandList->SetDynamicConstantBufferView(0, sizeof(m_BlurParamCB), &m_BlurParamCB);

	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[3] = {
		pLinearDepthBuffer->GetSRV(),
		pNormBuffer->GetSRV(),
		pDdxyBuffer->GetSRV()
	};
	pCommandList->SetDynamicDescriptors(2, 0, 3, Srvs);

	__RenderSingleDirectionFiltering(true, BlurRadius, pInputBuffer, &m_TemporalBuffer, pCommandList);
	__RenderSingleDirectionFiltering(false, BlurRadius, &m_TemporalBuffer, pOutputBuffer, pCommandList);

	for (UINT i = 1; i < BlurCount; i++)
	{
		__RenderSingleDirectionFiltering(true, BlurRadius, pOutputBuffer, &m_TemporalBuffer, pCommandList);
		__RenderSingleDirectionFiltering(false, BlurRadius, &m_TemporalBuffer, pOutputBuffer, pCommandList);
	}
}

void CBilateralBlurEffect::__BuildRootSignature()
{
	m_RootSignature.Reset(5, 2);
	m_RootSignature.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerPointClampDesc);
	m_RootSignature.InitStaticSampler(1, 0, GraphicsCommon::Sampler::SamplerLinearClampDesc);
	m_RootSignature[0].InitAsConstantBuffer(0, 0);
	m_RootSignature[1].InitAsConstants(1, 1, 0);
	m_RootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, 0);
	m_RootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, 0);
	m_RootSignature[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
	m_RootSignature.Finalize(L"BilateralBlurEffect", D3D12_ROOT_SIGNATURE_FLAG_NONE);
}

void CBilateralBlurEffect::__InitBlurPsShaders()
{
	m_BlurPsShaders[EBlurChannelType::kRed][0].first = g_pBilateralBlurEffectRed3x3CS;
	m_BlurPsShaders[EBlurChannelType::kRed][0].second = sizeof(g_pBilateralBlurEffectRed3x3CS);
	m_BlurPsShaders[EBlurChannelType::kRed][1].first = g_pBilateralBlurEffectRed5x5CS;
	m_BlurPsShaders[EBlurChannelType::kRed][1].second = sizeof(g_pBilateralBlurEffectRed5x5CS);
	m_BlurPsShaders[EBlurChannelType::kRed][2].first = g_pBilateralBlurEffectRed7x7CS;
	m_BlurPsShaders[EBlurChannelType::kRed][2].second = sizeof(g_pBilateralBlurEffectRed7x7CS);
	m_BlurPsShaders[EBlurChannelType::kRed][3].first = g_pBilateralBlurEffectRed9x9CS;
	m_BlurPsShaders[EBlurChannelType::kRed][3].second = sizeof(g_pBilateralBlurEffectRed9x9CS);
	m_BlurPsShaders[EBlurChannelType::kRed][4].first = g_pBilateralBlurEffectRed11x11CS;
	m_BlurPsShaders[EBlurChannelType::kRed][4].second = sizeof(g_pBilateralBlurEffectRed11x11CS);
	m_BlurPsShaders[EBlurChannelType::kRed][5].first = g_pBilateralBlurEffectRed13x13CS;
	m_BlurPsShaders[EBlurChannelType::kRed][5].second = sizeof(g_pBilateralBlurEffectRed13x13CS);
	m_BlurPsShaders[EBlurChannelType::kRed][6].first = g_pBilateralBlurEffectRed15x15CS;
	m_BlurPsShaders[EBlurChannelType::kRed][6].second = sizeof(g_pBilateralBlurEffectRed15x15CS);
}

void CBilateralBlurEffect::__BuildPSO(DXGI_FORMAT InputFormat)
{
	m_BlurChannelType = __GetBlurChannelType(InputFormat);
	auto PsShader = __GetBlurPsShaderCode(m_BlurChannelType, m_BlurRadius);
	_ASSERTE(PsShader.first);
	
	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetComputeShader(PsShader.first, PsShader.second);
	m_PSO.Finalize();
}

void CBilateralBlurEffect::__RenderSingleDirectionFiltering(bool HorizontalBlur, UINT BlurRadius, CColorBuffer* pInputBuffer, CColorBuffer* pOutputBuffer, CComputeCommandList* pCommandList)
{
	_ASSERTE(pInputBuffer && pOutputBuffer && pCommandList);

	pCommandList->TransitionResource(*pInputBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*pOutputBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pCommandList->ClearUAV(*pOutputBuffer);
	pCommandList->SetConstant(1, HorizontalBlur, 0);
	pCommandList->SetDynamicDescriptor(3, 0, pInputBuffer->GetSRV());
	pCommandList->SetDynamicDescriptor(4, 0, pOutputBuffer->GetUAV());
	XMUINT2 groupSize(CeilDivide(m_BlurImageSize.x, 8), CeilDivide(m_BlurImageSize.y, 8));
	pCommandList->Dispatch(groupSize.x, groupSize.y, 1);
}

CBilateralBlurEffect::EBlurChannelType CBilateralBlurEffect::__GetBlurChannelType(DXGI_FORMAT InputFormat)
{
	EBlurChannelType BlurChannelType;
	switch (InputFormat)
	{
	/*case DXGI_FORMAT_R32G32B32A32_FLOAT:
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
		BlurChannelType = EAWTBlurChannelType::kRgba;
		break;*/

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

const std::pair<const BYTE*, size_t>& CBilateralBlurEffect::__GetBlurPsShaderCode(EBlurChannelType BlurChannelType, UINT BlurRadius)
{
	_ASSERTE(BlurChannelType != EBlurChannelType::nChannelType);
	_ASSERTE(BlurRadius > 0 && BlurRadius <= MAX_BILATERAL_BLUR_RADIUS);
	return m_BlurPsShaders[BlurChannelType][BlurRadius - 1];
}
