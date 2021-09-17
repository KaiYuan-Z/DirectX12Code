#pragma once
#include "../Core/CameraBasic.h"
#include "../Core/GraphicsCore.h"
#include "../Core/GraphicsCommon.h"
#include "../Core/DoubleColorBuffer.h"

class CTemporalEffect
{
	struct SPerFrameCB
	{
		float Alpha = 0.1f;
		float ColorBoxSigma = 1.0f;
		XMFLOAT2 Pad;
	};

public:
	CTemporalEffect();
	~CTemporalEffect();

	void Init(UINT width, UINT height, CCameraBasic* pHostCamera);
	void OnFrameUpdate(UINT frameID);
	void AccumFrame(CColorBuffer* pCurColorBuffer, CColorBuffer* pCurMotionBuffer, CGraphicsCommandList* pGraphicsCommandList);

	CColorBuffer& GetCurrentAccumFrame() { return m_AccumColorBuffer.GetBackBuffer(); }

private:
	CCameraBasic* m_pHostCamera = nullptr;

	UINT m_WinWidth = 0;
	UINT m_WinHeight = 0;

	// If we want to jitter the camera to antialias using traditional a traditional 8x MSAA pattern, 
	// use these positions (which are in the range [-8.0...8.0], so divide by 16 before use)
	const float m_MSAA[8][2] = { {1,-3}, {-1,3}, {5,1}, {-3,-5}, {-5,5}, {-7,-1}, {3,7}, {7,-7} };

	CGraphicsPSO m_PSO;
	CRootSignature m_RootSignature;

	D3D12_VIEWPORT m_ViewPort;
	D3D12_RECT m_ScissorRect;

	CStructuredBuffer m_QuadVertexBuffer;
	CByteAddressBuffer m_QuadIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_QuadVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_QuadIndexBufferView;

	CDoubleColorBuffer m_AccumColorBuffer;

	SPerFrameCB m_PerFrameCB;

	void __InitQuadGeomtry();
	void __BuildRootSignature();
	void __BuildPSO();

	void __UpdateGUI();
};

