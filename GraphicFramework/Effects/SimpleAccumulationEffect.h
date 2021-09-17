#pragma once
#include "../Core/StructuredBuffer.h"
#include "../Core/ByteAddressBuffer.h"
#include "../Core/GraphicsCore.h"


class CSimpleAccumulationEffect
{
	struct SAccumulationCB
	{
		UINT AccumCount = 0;
	};

public:

	CSimpleAccumulationEffect();

	~CSimpleAccumulationEffect();

	void Init(DXGI_FORMAT frameFormat, UINT frameWidth, UINT frameHeight, UINT maxAccumCount);

	void Render(CColorBuffer* pLastFrame, CColorBuffer* pCurFrame, CColorBuffer* pOutputTarget, CGraphicsCommandList* pGraphicsCommandList);

	void ResetAccumCount() { m_AccumulationCB.AccumCount = 0; }

	UINT GetAccumCount() const { return m_AccumulationCB.AccumCount; }

private:
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	CGraphicsPSO m_Pso;
	CRootSignature m_RootSignature;

	CStructuredBuffer m_QuadVertexBuffer;
	CByteAddressBuffer m_QuadIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_QuadVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_QuadIndexBufferView;

	UINT m_MaxAccumCount = UINT_MAX;
	SAccumulationCB m_AccumulationCB;

	DXGI_FORMAT m_FrameFormat;

	void __InitQuadGeomtry();
	void __BuildRootSignature();
	void __BuildPSO();
};

