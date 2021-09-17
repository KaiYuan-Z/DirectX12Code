#pragma once
#include "../Core/DXSample.h"
#include "../Core/MathUtility.h"
#include "../Core/CameraBasic.h"
#include "../Core/StructuredBuffer.h"
#include "../Core/ByteAddressBuffer.h"
#include "../Core/ColorBuffer.h"

using namespace DirectX;

struct SSkyEffectConfig
{
    std::wstring CubeMapName;
	CCameraBasic* pMainCamera = nullptr;
	DXGI_FORMAT RenderTargetFormat;
};

class CSkyEffect
{
	struct SSkyCB
	{
		XMMATRIX ViewProj = XMMatrixIdentity();
		XMFLOAT3 CamPosW = {0.0f, 0.0f, 0.0f};
		float Pad;
	};

public:
	CSkyEffect();
	~CSkyEffect();

	void Init(const SSkyEffectConfig& config);
	void Render(bool clearRenderTarget, CColorBuffer* pRenderTarget, CGraphicsCommandList* pCommandList);

private:

	SSkyCB m_SkyCB;

	CGraphicsPSO m_PSO;
	CRootSignature m_RootSignature;

	D3D12_VIEWPORT m_ViewPort;
	D3D12_RECT m_ScissorRect;

	CStructuredBuffer m_SphereVertexBuffer;
	CByteAddressBuffer m_SphereIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_SphereVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_SphereIndexBufferView;

	D3D12_CPU_DESCRIPTOR_HANDLE m_CubeMapSrv;

	SSkyEffectConfig m_Config;

	void __InitSphereGeomtry();
	void __BuildRootSignature();
	void __BuildPSO();
};
