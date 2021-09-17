#pragma once
#include "../Core/DXSample.h"
#include "../Core/MathUtility.h"
#include "../Core/CameraBasic.h"
#include "../Core/StructuredBuffer.h"
#include "../Core/ColorBuffer.h"

#define MAX_BILATERAL_BLUR_RADIUS 7

using namespace DirectX;

class CBilateralBlurEffect
{
	struct SBlurParamCB
	{
		XMUINT2		ImageSize;
		XMFLOAT2	InvImageSize;

		float		DepthSigma;
		float		DepthThreshold;
		float		NormalSigma;
		float		NormalThreshold;
	};

	enum EBlurChannelType
	{
		kRed,
		//kRgba,

		nChannelType
	};

public:
	CBilateralBlurEffect();
	~CBilateralBlurEffect();

	void Init(DXGI_FORMAT InputFormat, UINT InputWidth, UINT InputHeight, float DepthSigma = 1.0f, float DepthThreshold = 0.1f, float NormalSigma = 128.0f, float NormalThreshold = 0.8f);

	void BlurImage(UINT BlurCount, 
		UINT BlurRadius, 
		CColorBuffer* pLinearDepthBuffer, 
		CColorBuffer* pNormBuffer,
		CColorBuffer* pDdxyBuffer,
		CColorBuffer* pInputBuffer, 
		CColorBuffer* pOutputBuffer, 
		CComputeCommandList* pCommandList);

private:
	std::pair<const BYTE*, size_t> m_BlurPsShaders[EBlurChannelType::nChannelType][MAX_BILATERAL_BLUR_RADIUS];

	EBlurChannelType m_BlurChannelType;

	UINT m_BlurRadius = 1;
	XMUINT2 m_BlurImageSize;

	CColorBuffer m_TemporalBuffer;

	SBlurParamCB m_BlurParamCB;

	CComputePSO m_PSO;
	CRootSignature m_RootSignature;

	void __BuildRootSignature();
	void __InitBlurPsShaders();
	void __BuildPSO(DXGI_FORMAT InputFormat);
	void __RenderSingleDirectionFiltering(bool HorizontalBlur, UINT BlurRadius, CColorBuffer* pInputBuffer, CColorBuffer* pOutputBuffer, CComputeCommandList* pCommandList);

	EBlurChannelType __GetBlurChannelType(DXGI_FORMAT InputFormat);
	const std::pair<const BYTE*, size_t>& __GetBlurPsShaderCode(EBlurChannelType BlurChannelType, UINT BlurRadius);
};
