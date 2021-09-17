#pragma once
#include "../Core/DXSample.h"
#include "../Core/MathUtility.h"
#include "../Core/CameraBasic.h"
#include "../Core/StructuredBuffer.h"
#include "../Core/ByteAddressBuffer.h"
#include "../Core/Model.h"
#include "../Core/ColorBuffer.h"
#include "../Core/Texture.h"

using namespace DirectX;


class CSsaoEffect
{
	struct SConstantBufferPerFrame
	{
		XMMATRIX View;
		XMMATRIX Proj;
		XMMATRIX InvProj;
		XMMATRIX ProjTex;
	};

	struct SConstantBufferSsao
	{
		// Coordinates given in view space.
		float OcclusionRadius;
		float OcclusionFadeStart;
		float OcclusionFadeEnd;
		float SurfaceEpsilon;
		int SampleCount;
		int FrameCount;
		int SsaoPower;
		bool UseTemporalRandom;
	};

public:
	CSsaoEffect();
	~CSsaoEffect();

	void Init(DXGI_FORMAT SsaoMapFormat, UINT SsaoMapWidth, UINT SsaoMapHeight, bool UseTemporalRandom = false);
	void Render(const CCameraBasic* pCamera, CColorBuffer* pInputNormMap, CDepthBuffer* pInputDepthMap, CColorBuffer* pOutputSsaoMap, CGraphicsCommandList* pCommandList);

private:

	SConstantBufferPerFrame m_CBPerFrame;
	SConstantBufferSsao m_CBSsao;

	CGraphicsPSO m_PSO;
	CRootSignature m_RootSignature;

	D3D12_VIEWPORT m_ViewPort;
	D3D12_RECT m_ScissorRect;

	DXGI_FORMAT m_SsaoMapFormat;

	CStructuredBuffer m_QuadVertexBuffer;
	CByteAddressBuffer m_QuadIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_QuadVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_QuadIndexBufferView;

	bool m_UseTemporalRandom = false;

	void __InitQuadGeomtry();
	void __BuildConstantBuffer();
	void __BuildRootSignature();
	void __BuildPSO();

	void __UpdateGUI();
};
