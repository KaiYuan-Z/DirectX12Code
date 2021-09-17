#pragma once
#include "../Core/MathUtility.h"
#include "../Core/ColorBuffer.h"
#include "../Core/GraphicsCoreBase.h"
#include "../Core/GraphicsCore.h"


class CPartialDerivativesEffect
{
public:

	CPartialDerivativesEffect();
	~CPartialDerivativesEffect();

	void Init();

	void CalcPartialDerivatives(CColorBuffer* pInputTex, CColorBuffer* pOutputPartialDerivatives, CComputeCommandList* pComputeCommandList);

private:
	CComputePSO m_Pso;
	CRootSignature m_RootSignature;

	void __BuildRootSignature();
	void __BuildPSO();
};

