#include "AtrousWaveletTransfromBlurEffect.h"
#include "../Core/GraphicsCommon.h"

#include "CompiledShaders/AtrousWaveletTransfromBlurEffectRed3x3CS.h"
#include "CompiledShaders/AtrousWaveletTransfromBlurEffectRed5x5CS.h"
#include "CompiledShaders/AtrousWaveletTransfromBlurEffectRed7x7CS.h"
#include "CompiledShaders/AtrousWaveletTransfromBlurEffectRed9x9CS.h"
#include "CompiledShaders/AtrousWaveletTransfromBlurEffectRed11x11CS.h"


CAtrousWaveletTransfromBlurEffect::CAtrousWaveletTransfromBlurEffect()
{
}

CAtrousWaveletTransfromBlurEffect::~CAtrousWaveletTransfromBlurEffect()
{
}

void CAtrousWaveletTransfromBlurEffect::Init(DXGI_FORMAT InputFormat, UINT InputWidth, UINT InputHeight)
{
	m_TemporalBuffer.Create2D(L"BlurTemporalBuffer", InputWidth, InputHeight, 1, InputFormat);

	m_BlurImageSize = XMUINT2(InputWidth, InputHeight);

	m_BlurParamCB.TextureDim = m_BlurImageSize;

	__BuildRootSignature();
	__InitBlurPsShaders();
	__BuildPSO(InputFormat);
}

void CAtrousWaveletTransfromBlurEffect::BlurImage(UINT BlurCount,
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
	pCommandList->SetDynamicDescriptors(1, 0, 3, Srvs);

	__RenderSingleFiltering(BlurRadius, pInputBuffer, pOutputBuffer, pCommandList);

	if (BlurCount > 1)
	{
		auto* pInput = pOutputBuffer;
		auto* pOutput = &m_TemporalBuffer;
		for (UINT i = 1; i < BlurCount; i++)
		{
			__RenderSingleFiltering(BlurRadius, pInput, pOutput, pCommandList);
			std::swap(pInput, pOutput);
		}

		if (BlurCount % 2 == 0)
		{
			pCommandList->TransitionResource(m_TemporalBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
			pCommandList->TransitionResource(*pOutputBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
			pCommandList->CopySubresource(*pOutputBuffer, 0, m_TemporalBuffer, 0);
		}
	}
}

void CAtrousWaveletTransfromBlurEffect::__BuildRootSignature()
{
	m_RootSignature.Reset(4, 0);
	m_RootSignature[0].InitAsConstantBuffer(0, 0);
	m_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, 0);
	m_RootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, 0);
	m_RootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
	m_RootSignature.Finalize(L"AtrousWaveletTransfromBlurEffect", D3D12_ROOT_SIGNATURE_FLAG_NONE);
}

void CAtrousWaveletTransfromBlurEffect::__InitBlurPsShaders()
{
	m_BlurPsShaders[EBlurChannelType::kRed][0].first = g_pAtrousWaveletTransfromBlurEffectRed3x3CS;
	m_BlurPsShaders[EBlurChannelType::kRed][0].second = sizeof(g_pAtrousWaveletTransfromBlurEffectRed3x3CS);
	m_BlurPsShaders[EBlurChannelType::kRed][1].first = g_pAtrousWaveletTransfromBlurEffectRed5x5CS;
	m_BlurPsShaders[EBlurChannelType::kRed][1].second = sizeof(g_pAtrousWaveletTransfromBlurEffectRed5x5CS);
	m_BlurPsShaders[EBlurChannelType::kRed][2].first = g_pAtrousWaveletTransfromBlurEffectRed7x7CS;
	m_BlurPsShaders[EBlurChannelType::kRed][2].second = sizeof(g_pAtrousWaveletTransfromBlurEffectRed7x7CS);
	m_BlurPsShaders[EBlurChannelType::kRed][3].first = g_pAtrousWaveletTransfromBlurEffectRed9x9CS;
	m_BlurPsShaders[EBlurChannelType::kRed][3].second = sizeof(g_pAtrousWaveletTransfromBlurEffectRed9x9CS);
	m_BlurPsShaders[EBlurChannelType::kRed][4].first = g_pAtrousWaveletTransfromBlurEffectRed11x11CS;
	m_BlurPsShaders[EBlurChannelType::kRed][4].second = sizeof(g_pAtrousWaveletTransfromBlurEffectRed11x11CS);
}

void CAtrousWaveletTransfromBlurEffect::__BuildPSO(DXGI_FORMAT InputFormat)
{
	m_BlurChannelType = __GetBlurChannelType(InputFormat);
	auto PsShader = __GetBlurPsShaderCode(m_BlurChannelType, m_BlurRadius);
	_ASSERTE(PsShader.first);
	
	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetComputeShader(PsShader.first, PsShader.second);
	m_PSO.Finalize();
}

void CAtrousWaveletTransfromBlurEffect::__RenderSingleFiltering(UINT BlurRadius, CColorBuffer* pInputBuffer, CColorBuffer* pOutputBuffer, CComputeCommandList* pCommandList)
{
	_ASSERTE(pInputBuffer && pOutputBuffer && pCommandList);

	pCommandList->TransitionResource(*pInputBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*pOutputBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
	pCommandList->ClearUAV(*pOutputBuffer);
	pCommandList->SetDynamicDescriptor(2, 0, pInputBuffer->GetSRV());
	pCommandList->SetDynamicDescriptor(3, 0, pOutputBuffer->GetUAV());
	XMUINT2 groupSize(CeilDivide(m_BlurImageSize.x, 8), CeilDivide(m_BlurImageSize.y, 8));
	pCommandList->Dispatch(groupSize.x, groupSize.y, 1);
}

CAtrousWaveletTransfromBlurEffect::EBlurChannelType CAtrousWaveletTransfromBlurEffect::__GetBlurChannelType(DXGI_FORMAT InputFormat)
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

const std::pair<const BYTE*, size_t>& CAtrousWaveletTransfromBlurEffect::__GetBlurPsShaderCode(EBlurChannelType BlurChannelType, UINT BlurRadius)
{
	_ASSERTE(BlurChannelType != EBlurChannelType::nChannelType);
	_ASSERTE(BlurRadius > 0 && BlurRadius <= MAX_AWT_BLUR_RADIUS);
	return m_BlurPsShaders[BlurChannelType][BlurRadius - 1];
}
