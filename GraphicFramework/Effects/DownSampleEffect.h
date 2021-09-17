#pragma once
#include "../Core/MathUtility.h"
#include "../Core/ColorBuffer.h"
#include "../Core/GraphicsCoreBase.h"
#include "../Core/GraphicsCore.h"


class CDownSampleEffect
{
public:

	CDownSampleEffect();
	~CDownSampleEffect();

	void Init();

	void DownSampleRed2x2(CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList);
	void DownSampleRed3x3(CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList);
	void DownSampleRed4x4(CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList);

	void DownSampleRg2x2(CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList);
	void DownSampleRg3x3(CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList);
	void DownSampleRg4x4(CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList);

private:
	CComputePSO m_PsoRed2x2;
	CComputePSO m_PsoRed3x3;
	CComputePSO m_PsoRed4x4;

	CComputePSO m_PsoRg2x2;
	CComputePSO m_PsoRg3x3;
	CComputePSO m_PsoRg4x4;

	CRootSignature m_RootSignature;

	void __BuildRootSignature();
	void __BuildPSO();

	void __DownSample(UINT Size, CColorBuffer* pInputTex, CColorBuffer* pOutputTex, CComputeCommandList* pComputeCommandList);
};

