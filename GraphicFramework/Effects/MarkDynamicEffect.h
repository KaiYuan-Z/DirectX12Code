#pragma once
#include "../Core/GraphicsCoreBase.h"
#include "../Core/GraphicsCore.h"
#include "../Core/ColorBuffer.h"
#include "../Core/StructuredBuffer.h"
#include "../Core/Scene.h"


struct SMarkDynamicConfig
{	
	UINT WinWidth = 0;
	UINT WinHeight = 0;
	DXGI_FORMAT DepthStencilBufferFormat = DXGI_FORMAT_D32_FLOAT;
	UINT ActiveCameraIndex = 0;
	CScene* pScene = nullptr;
	bool EnableGUI = true;
};

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

class CMarkDynamicEffect
{
public:
	CMarkDynamicEffect();
	~CMarkDynamicEffect();

	void Init(const SMarkDynamicConfig& config);

	void Render(float extent, CDepthBuffer* pCurrentDepth, CGraphicsCommandList *pCommandList);

	CColorBuffer& GetDynamicMarkMap() { return m_DynamicMarkMap; }

private:
	CColorBuffer m_DynamicMarkMap;

	SMarkDynamicConfig m_MarkDynamicConfig;
	float m_ExtentAppend = 1.0f;
	bool m_MarkPreFrameDynamicInstance = true;

	std::vector<XMMATRIX> m_OBBFinnals;
	std::vector<int> m_OBBTags;

	CStructuredBuffer m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	CByteAddressBuffer m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	CGraphicsPSO m_PSO1;
	CGraphicsPSO m_PSO2;
	CRootSignature m_RootSignature;

	D3D12_VIEWPORT m_ViewPort;
	D3D12_RECT m_ScissorRect;

	void __InitRootRootSignature();
	void __InitPSO();
	void __InitGeometry();
	XMMATRIX __CreateOBBFinal(const SBoundingObject& boundingObject, float extent);
	void __UpdateGUI();
};

