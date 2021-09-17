#pragma once
#include "../Core/DXSample.h"
#include "../Core/MathUtility.h"
#include "../Core/CameraBasic.h"
#include "../Core/StructuredBuffer.h"
#include "../Core/ColorBuffer.h"

#define MAX_BLUR_RADIUS 7

using namespace DirectX;

class CBlurEffect
{
	struct SBlurParamCB
	{
		XMFLOAT2 InvImageSize;
		XMFLOAT2 Pad;
	};

	enum EBlurChannelType
	{
		kRed,
		kRgba,

		nChannelType
	};

public:
	CBlurEffect();
	~CBlurEffect();

	void Init(DXGI_FORMAT InputFormat, UINT InputWidth, UINT InputHeight);
	void BlurImage(UINT BlurCount, UINT BlurRadius, CColorBuffer* pLinearDepthBuffer, CColorBuffer* pInputBuffer, CColorBuffer* pOutputBuffer, CComputeCommandList* pCommandList);

private:
	std::pair<const BYTE*, size_t> m_BlurPsShaders[EBlurChannelType::nChannelType][MAX_BLUR_RADIUS];

	EBlurChannelType m_BlurChannelType;

	UINT m_BlurRadius = 1;
	XMUINT2 m_BlurImageSize;

	CColorBuffer m_TemporalBuffer;

	CStructuredBuffer m_BlurParamCB;

	CComputePSO m_PSO;
	CRootSignature m_RootSignature;

	CStructuredBuffer m_QuadVertexBuffer;
	CByteAddressBuffer m_QuadIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_QuadVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_QuadIndexBufferView;

	void __BuildRootSignature();
	void __InitBlurPsShaders();
	void __BuildPSO(DXGI_FORMAT InputFormat);
	void __RenderSingleDirectionFiltering(bool OpenHorizontalBlur, UINT BlurRadius, CColorBuffer* pInputBuffer, CColorBuffer* pOutputBuffer, CComputeCommandList* pCommandList);

	EBlurChannelType __GetBlurChannelType(DXGI_FORMAT InputFormat);
	const std::pair<const BYTE*, size_t>& __GetBlurPsShaderCode(EBlurChannelType BlurChannelType, UINT BlurRadius);
};
