#pragma once
#include "../Core/DXSample.h"
#include "../Core/MathUtility.h"
#include "../Core/CameraBasic.h"
#include "../Core/StructuredBuffer.h"
#include "../Core/ColorBuffer.h"

#define MAX_AWT_BLUR_RADIUS 5

using namespace DirectX;

class CAtrousWaveletTransfromBlurEffect
{
	struct SBlurParamCB
	{
		XMUINT2		TextureDim;
		float		DepthWeightCutoff = 0.2f;
		float		DepthSigma = 1.0f;

		float		NormalSigma = 128;
		XMUINT3		Pad;
	};

	enum EBlurChannelType
	{
		kRed,
		//kRgba,

		nChannelType
	};

public:
	CAtrousWaveletTransfromBlurEffect();
	~CAtrousWaveletTransfromBlurEffect();

	void Init(DXGI_FORMAT InputFormat, UINT InputWidth, UINT InputHeight);

	void BlurImage(UINT BlurCount, 
		UINT BlurRadius, 
		CColorBuffer* pLinearDepthBuffer, 
		CColorBuffer* pNormBuffer,
		CColorBuffer* pDdxyBuffer,
		CColorBuffer* pInputBuffer, 
		CColorBuffer* pOutputBuffer, 
		CComputeCommandList* pCommandList);

private:
	std::pair<const BYTE*, size_t> m_BlurPsShaders[EBlurChannelType::nChannelType][MAX_AWT_BLUR_RADIUS];

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
	void __RenderSingleFiltering(UINT BlurRadius, CColorBuffer* pInputBuffer, CColorBuffer* pOutputBuffer, CComputeCommandList* pCommandList);

	EBlurChannelType __GetBlurChannelType(DXGI_FORMAT InputFormat);
	const std::pair<const BYTE*, size_t>& __GetBlurPsShaderCode(EBlurChannelType BlurChannelType, UINT BlurRadius);
};
