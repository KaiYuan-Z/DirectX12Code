#pragma once
#include "../Core/GraphicsCore.h"
#include "../Core/GraphicsCommon.h"

class CFxaaEffect
{
	struct SFxaaParamters
	{
		XMFLOAT2 InvTexDim;
		float QualitySubPix = 1.0f;

		float QualityEdgeThreshold = 0.05f;
		float QualityEdgeThresholdMin = 0.01f;
		bool EarlyOut = true;
		float pad;
	};

public:
	CFxaaEffect();
	~CFxaaEffect();

	void Init(DXGI_FORMAT colorBufferFormat);
	void RenderFxaa(CColorBuffer* pInColorBuffer, CColorBuffer* pOutColorBuffer, CGraphicsCommandList* pGraphicsCommandList);

private:
	CGraphicsPSO m_PSO;
	CRootSignature m_RootSignature;

	CStructuredBuffer m_QuadVertexBuffer;
	CByteAddressBuffer m_QuadIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_QuadVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_QuadIndexBufferView;

	SFxaaParamters m_ParamtersCB;

	void __InitQuadGeomtry();
	void __BuildRootSignature();
	void __BuildPSO(DXGI_FORMAT colorBufferFormat);
};

