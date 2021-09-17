#pragma once
#include "../Core/MathUtility.h"
#include "../Core/ColorBuffer.h"
#include "../Core/DepthBuffer.h"
#include "../Core/CommandList.h"
#include "../Core/StructuredBuffer.h"
#include "../Core/ByteAddressBuffer.h"

enum EImageEffectMode
{
	kRGBA,
	kRED,
	kAccumMap,
	kDynamicMark
};

class CImageEffect
{
	struct SSsaaCB
	{
		UINT SsaaRadius = 0;
		UINT HasCenter = 0;
		UINT SrvWidth = 0;
		UINT SrvHeight = 0;
	};

public:

	CImageEffect();
	~CImageEffect();

	void Init(float WinScale);
	void Init(float WinScale, float TopLeftXOffset, float TopLeftYOffset);
	void Render(EImageEffectMode Mode, CColorBuffer* pSRV, CGraphicsCommandList* pGraphicsCommandList);
	void RenderSsaa(EImageEffectMode Mode, CColorBuffer* pSRV, CGraphicsCommandList* pGraphicsCommandList);

private:

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
	
	CGraphicsPSO m_PsoRed;
	CGraphicsPSO m_PsoRgba;
	CGraphicsPSO m_PsoSsaaRed;
	CGraphicsPSO m_PsoSsaaRgba;
	CGraphicsPSO m_PsoAccumCount;
	CGraphicsPSO m_PsoDynamicMark;

	CRootSignature m_RootSignatureGeneral;
	CRootSignature m_RootSignatureSsaa;
	CRootSignature m_RootSignatureAccumCount;
	CRootSignature m_RootSignatureDynamicMark;

	CStructuredBuffer m_QuadVertexBuffer;
	CByteAddressBuffer m_QuadIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_QuadVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_QuadIndexBufferView;

	SSsaaCB m_SsaaCB;

	void __InitQuadGeomtry();
	void __BuildRootSignature();
	void __BuildPSOs();
	void __UpdateSsaaCB(CColorBuffer* pSRV);
};

