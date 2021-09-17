#pragma once
#include "../Core/GraphicsCore.h"
#include "../Core/CameraBasic.h"
#include "../Core/DoubleColorBuffer.h"


class CAccumulationEffectTestID
{
	struct SAccumulationMapCB
	{
		XMMATRIX View = XMMatrixIdentity();
		bool     ViewUpdateFlag = false;
		XMFLOAT3 Pad;
	};

public:

	CAccumulationEffectTestID();
	~CAccumulationEffectTestID();

	void Init(DXGI_FORMAT frameFormat, UINT frameWidth, UINT frameHeight);

	void OnFrameUpdate(const CCameraBasic* pCamera);
	void CalculateAccumMap(CColorBuffer* pPreIdBuffer, CColorBuffer* pCurIdBuffer, CColorBuffer* pMotionBuffer, CGraphicsCommandList* pGraphicsCommandList);
	void AccumulateFrame(CColorBuffer* pCurFrameBuffer, CColorBuffer* pMotionBuffer, CGraphicsCommandList* pGraphicsCommandList);

	CColorBuffer& GetAccumMap() { return m_DoubleAccumMap.GetFrontBuffer(); }

	CColorBuffer& GetAccumFrameMap() { return m_DoubleFrameMap.GetFrontBuffer(); }

private:

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	CGraphicsPSO m_PsoAccumMap;
	CGraphicsPSO m_PsoAccumRed;
	CRootSignature m_RootSignatureAccumMap;
	CRootSignature m_RootSignatureAccumRed;

	CStructuredBuffer m_QuadVertexBuffer;
	CByteAddressBuffer m_QuadIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_QuadVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_QuadIndexBufferView;

	SAccumulationMapCB m_AccumulationMapCB;

	CDoubleColorBuffer m_DoubleFrameMap;
	CDoubleColorBuffer m_DoubleAccumMap;

	DXGI_FORMAT m_FrameFormat;

	void __InitQuadGeomtry();
	void __BuildRootSignature();
	void __BuildPSO();
	void __BuildMaps(UINT frameWidth, UINT frameHeight);
	void __VerifySRV(const CPixelBuffer* pSRV);
};

