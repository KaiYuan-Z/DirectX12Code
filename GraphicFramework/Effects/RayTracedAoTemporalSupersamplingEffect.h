#pragma once
#include "../Core/GraphicsCore.h"
#include "../Core/CameraBasic.h"
#include "../Core/DoubleColorBuffer.h"


class CRayTracedAoTemporalSupersamplingEffect
{
	struct SAccumMapCB
	{
		UINT	WinWidth;
		UINT	WinHeight;
		float	InvWinWidth;
		float	InvWinHeight;
	};

	struct SAccumAoCB
	{
		UINT		WinWidth = 0;
		UINT		WinHeight = 0;
		UINT		MaxAccumCount = 150;
		bool		ReUseDiffAo = true;

		float		AoDiffFactor1 = 2.0f;
		float		AoDiffFactor2 = 0.75f;
		XMFLOAT2	Pad;
	};

	struct SMixBluredAoCB
	{
		UINT    UseBlurMaxTspp = 32;
		float   BlurDecayStrength = 0.2f;
		XMFLOAT2	Pad;
	};

public:

	CRayTracedAoTemporalSupersamplingEffect();
	~CRayTracedAoTemporalSupersamplingEffect();

	void Init(DXGI_FORMAT outPutFrameFormat, UINT outPutFrameWidth, UINT outPutFrameHeight, bool enableGUI = true);

	void CalculateAccumMap(CColorBuffer* pPreLinearDepthBuffer, 
		CColorBuffer* pPreNormBuffer,
		CColorBuffer* pCurLinearDepthBuffer, 
		CColorBuffer* pCurNormBuffer,
		CColorBuffer* pCurDdxyBuffer,
		CColorBuffer* pCurMotionBuffer, 
		CColorBuffer* pCurIdBuffer, 
		CColorBuffer* pDynamicAreaMarkBuffer,
		CComputeCommandList* pComputeCommandList);

	void AccumulateFrame(UINT maxAccumCount, CColorBuffer* pCurFrameBuffer, CColorBuffer* pCurFrameDiff, CComputeCommandList* pComputeCommandList);

	void ClearMaps(CComputeCommandList* pComputeCommandList);

	void MixBluredResult(CColorBuffer* pBluredFrameBuffer, CColorBuffer* pAoDiffBuffer, CComputeCommandList* pComputeCommandList);

	CColorBuffer& GetCurAccumMap() { return m_DoubleAccumMap.GetFrontBuffer(); }
	CColorBuffer& GetCurAccumFrame() { return m_DoubleAccumFrame.GetFrontBuffer(); }
	CColorBuffer& GetCurMixBlurFrame() { return m_MixBlurFrame; }

private:
	CComputePSO m_PsoAccumMap;
	CComputePSO m_PsoAccumAo;
	CComputePSO m_PsoMixBluredAo;
	CRootSignature m_RootSignatureAccumMap;
	CRootSignature m_RootSignatureAccumAo;
	CRootSignature m_RootSignatureMixBluredAo;

	SAccumMapCB m_AccumMapCB;
	SAccumAoCB m_AccumAoCB;
	SMixBluredAoCB m_MixBluredAoCB;
	
	CColorBuffer m_MixBlurFrame;
	CDoubleColorBuffer m_DoubleAccumMap;
	CDoubleColorBuffer m_DoubleAccumFrame;

	DXGI_FORMAT m_OutputFrameFormat;
	XMUINT2 m_OutputFrameSize;

	void __BuildRootSignature();
	void __BuildPSO();
	void __BuildAccumMap();
	void __UpdateGUI();
};

