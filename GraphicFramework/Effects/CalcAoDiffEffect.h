#pragma once
#include "../RaytracingUtility/RaytracingScene.h"
#include "../RaytracingUtility/RaytracingSceneRenderHelper.h"
#include "../RaytracingUtility/RayTracingPipelineStateObject.h"
#include "../RaytracingUtility/RaytracingDispatchRaysDescriptor.h"
#include "../Core/DoubleColorBuffer.h"
#include "../Core/CameraBasic.h"

#include "BlurEffect.h"
#include "MeanVarianceEffect.h"
#include "BilateralBlurEffect.h"
#include "DownSampleEffect.h"


class CCalcAoDiffEffect
{
	struct SRayGenCB
	{
		XMMATRIX	PreInvViewProj = XMMatrixIdentity();

		XMFLOAT4	PreCameraPos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

		float		AoRadius = 20.0f;
		float		MinT = 0.02f;
		float       AoDiffScale = 8.0f;
		UINT		OpenAoDiffTest = 1;

		UINT        UseDynamicMark = 1;
		XMFLOAT3    Pad;
	};

	struct SDownsampleAoDiffCB
	{
		XMINT2 AoMapSize = XMINT2(0, 0);
		XMINT2 Pad;
	};

public:
	CCalcAoDiffEffect();
	~CCalcAoDiffEffect();

	UINT GetDiffMapWidth() const { return m_AoDiffWidth; }
	UINT GetDiffMapHeight() const { return m_AoDiffHeight; }

	void Init(RaytracingUtility::CRaytracingScene* pRtScene, UINT activeCameraIndex, UINT aoMapWidth, UINT aoMapHeight, DXGI_FORMAT aoMapFormat, bool enableGUI = true);

	void Update(CCameraBasic* pCamera, float aoRadius, float aoMinT);

	void CalcAoDiff(CColorBuffer* pPreHaltonIndex,
		CColorBuffer* pPreSampleCacheBuffer,
		CColorBuffer* pPreIdBuffer,		
		CColorBuffer* pCurMotionVecBuffer,
		CColorBuffer* pCurAccumMapBuffer,
		CComputeCommandList* pComputeCommandList);

	void CalcAoDiff(CColorBuffer* pPreHaltonIndex,
		CColorBuffer* pPreSampleCacheBuffer,
		CColorBuffer* pPreIdBuffer,
		CColorBuffer* pCurLinearDepthBuffer,
		CColorBuffer* pCurMotionVecBuffer,
		CColorBuffer* pCurAccumMapBuffer,
		CComputeCommandList* pComputeCommandList);

	void CalcAoDiff(CColorBuffer* pPreHaltonIndex,
		CColorBuffer* pPreSampleCacheBuffer,
		CColorBuffer* pPreIdBuffer,
		CColorBuffer* pCurLinearDepthBuffer,
		CColorBuffer* pCurNormBuffer,
		CColorBuffer* pDdxyBuffer,
		CColorBuffer* pCurMotionVecBuffer,
		CColorBuffer* pCurAccumMapBuffer,
		CComputeCommandList* pComputeCommandList);

	CColorBuffer& GetAoDiffMap() { return m_AoDiffMap.GetBackBuffer(); }

	void SetOpenState(bool open);
	bool GetOpenState() const { return m_RayGenCB.OpenAoDiffTest; }

	void UseDynamicAoMark(bool open);

private:
	// Pipline configs for GenTestRays.
	RaytracingUtility::CRaytracingSceneRenderHelper m_RtSceneRender;
	RaytracingUtility::CRayTracingPipelineStateObject m_RayTracingStateObject;
	RaytracingUtility::CRaytracingDispatchRaysDescriptor m_DispatchRaysDesc;
	CRootSignature m_RayTracingRootSignature;
	CRootSignature m_RaytracingLocalRootSignature;
	SRayGenCB m_RayGenCB;
	UINT m_RaytracingUserRootParamStartIndex = 0;

	// Pipline configs for CalcAoDiffMap.
	CComputePSO m_PsoCalcAoDiffMap;
	CRootSignature m_RootSignatureCalcAoDiffMap;
	SDownsampleAoDiffCB m_DownsampleAoDiffCB;

	CBlurEffect m_BlurAoDiffEffect;
	CMeanVarianceEffect m_MeanAoDiffEffect;
	CBilateralBlurEffect m_BilateralBlurAoDiffEffect;
	CDownSampleEffect m_DownSampleEffect;

	CColorBuffer m_DownSampledLinearDepthBuffer;
	CColorBuffer m_DownSampledNormBuffer;
	CColorBuffer m_DownSampledDdxyBuffer;
	CColorBuffer m_RawAoDiffMap;
	CDoubleColorBuffer m_AoDiffMap;

	CByteAddressBuffer m_AoRayCounter;

	UINT m_AoDiffWidth = 0;
	UINT m_AoDiffHeight = 0;
	UINT m_AoDiffBlurCount = 5;
	UINT m_AoDiffBlurRadius = 7;

	void __InitGenTestRaysResources(RaytracingUtility::CRaytracingScene* pRtScene, UINT activeCameraIndex);

	void __InitCalcAoDiffMapResources();

	void __GenTestRays(CColorBuffer* pPreHaltonIndex,
		CColorBuffer* pPreSampleCacheBuffer,
		CColorBuffer* pPreIdBuffer,
		CColorBuffer* pCurMotionVecBuffer,
		CColorBuffer* pCurAccumMapBuffer,
		CComputeCommandList* pComputeCommandList);

	void __CalcAoDiffMap(CComputeCommandList* pComputeCommandList);

	void __UpdateGUI();
};

