#pragma once
#include "../Core/MathUtility.h"
#include "../Core/ColorBuffer.h"
#include "../Core/GraphicsCoreBase.h"
#include "../Core/GraphicsCore.h"


class CMeanVarianceEffect
{
	struct SMeanVarianceCB
	{
		XMUINT2 TextureDim;
		UINT KernelWidth;
		UINT KernelRadius;
	};

public:

	CMeanVarianceEffect();
	~CMeanVarianceEffect();

	void Init();

	void CalcMean(UINT kernelWidth, CColorBuffer* pInputTex, CColorBuffer* pOutputMean, CComputeCommandList* pComputeCommandList);
	void CalcVariance(UINT kernelWidth, CColorBuffer* pInputTex, CColorBuffer* pOutputVariance, CComputeCommandList* pComputeCommandList);
	void CalcMeanVariance(UINT kernelWidth, CColorBuffer* pInputTex, CColorBuffer* pOutputMeanVariance, CComputeCommandList* pComputeCommandList);

private:
	CComputePSO m_PsoMean;
	CComputePSO m_PsoVariance;
	CComputePSO m_PsoMeanVariance;
	CRootSignature m_RootSignatureMean;
	CRootSignature m_RootSignatureVariance;
	CRootSignature m_RootSignatureMeanVariance;
	SMeanVarianceCB m_MeanVarianceCB;

	void __BuildRootSignature();
	void __BuildPSO();
	void __Dispatch(UINT kernelWidth, UINT texWidth, UINT texHeight, CComputeCommandList* pComputeCommandList);
};

