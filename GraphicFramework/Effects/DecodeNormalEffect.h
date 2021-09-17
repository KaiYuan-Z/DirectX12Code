#pragma once
#include "../Core/MathUtility.h"
#include "../Core/ColorBuffer.h"
#include "../Core/GraphicsCoreBase.h"
#include "../Core/GraphicsCore.h"


class CDecodeNormalEffect
{
public:

	CDecodeNormalEffect();
	~CDecodeNormalEffect();

	void Init();

	void DecodeNormal(CColorBuffer* pInputFloat2Buffer, CColorBuffer* pOutputFloat4Buffer, CComputeCommandList* pComputeCommandList);

private:
	CComputePSO m_Pso;

	CRootSignature m_RootSignature;

	void __BuildRootSignature();
	void __BuildPSO();
};

