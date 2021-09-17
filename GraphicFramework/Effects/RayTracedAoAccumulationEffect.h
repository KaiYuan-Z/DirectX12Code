#pragma once
#include "../Core/GraphicsCore.h"
#include "../Core/CameraBasic.h"
#include "../Core/DoubleColorBuffer.h"


class CRayTracedAoAccumulationEffect
{
	struct SAccumMapCB
	{
		float	ReprojTestFactor = 0.05f;
		UINT	DynamicMarkMaxLife = 5;
		UINT	WinWidth = 0;
		UINT	WinHeight = 0;
	};

	struct SAccumAoCB
	{
		float	StdDevGamma = 0.6f;
		float	ClampMinStdDevTolerance = 0.0f;
		float	ClampDifferenceToTsppScale = 4.0f;
		float	MinSmoothingFactor = 0.3f;
		UINT	MinTsppToUseTemporalVariance = 16;
		UINT	MinNsppForClamp = 6;
		UINT	DynamicMarkMaxLife = 5;
		UINT	WinWidth = 0;
		UINT	WinHeight = 0;
		UINT	MaxAccumCount = 150;
		XMFLOAT2	Pad;
	};

	struct SMixBluredAoCB
	{
		UINT    UseBlurMaxTspp = 32;
		float   BlurDecayStrength = 0.2f;
		XMFLOAT2	Pad;
	};

public:

	CRayTracedAoAccumulationEffect();
	~CRayTracedAoAccumulationEffect();

	void Init(DXGI_FORMAT outPutFrameFormat, UINT outPutFrameWidth, UINT outPutFrameHeight);

	void CalculateAccumMap(CColorBuffer* pPreLinearDepthBuffer, 
		CColorBuffer* pCurLinearDepthBuffer, 
		CColorBuffer* pMotionBuffer, 
		CColorBuffer* pDynamicAreaMarkBuffer, 
		CColorBuffer*  pIdMapBuffer, 
		CComputeCommandList* pComputeCommandList);

	void AccumulateFrame(CColorBuffer* pCurFrameBuffer, 
		CColorBuffer* pMotionBuffer, 
		CColorBuffer* pLocalMeanVarianceBuffer, 
		CComputeCommandList* pComputeCommandList);

	void MixBluredResult(CColorBuffer* pBluredFrameBuffer, CComputeCommandList* pComputeCommandList);

	CColorBuffer& GetCurAccumMap() { return m_DoubleAccumMap.GetFrontBuffer(); }
	CColorBuffer& GetCurAccumFrame() { return m_DoubleAccumFrame.GetFrontBuffer(); }
	CColorBuffer& GetCurAccumVariance() { return m_AccumVariance; }
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
	
	CColorBuffer m_AccumVariance;
	CColorBuffer m_MixBlurFrame;
	CDoubleColorBuffer m_DoubleAccumMap;
	CDoubleColorBuffer m_DoubleAccumSquaredMean;
	CDoubleColorBuffer m_DoubleAccumFrame;

	DXGI_FORMAT m_OutputFrameFormat;
	XMUINT2 m_OutputFrameSize;

	void __InitQuadGeomtry();
	void __BuildRootSignature();
	void __BuildPSO();
	void __BuildAccumMap();
	void __UpdateGUI();
};

