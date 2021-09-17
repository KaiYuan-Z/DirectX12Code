#pragma once
#include "../Core/MathUtility.h"
#include "../Core/CameraBasic.h"
#include "../Core/ColorBuffer.h"
#include "../Core/DepthBuffer.h"
#include "../Core/CommandList.h"
#include "../Core/StructuredBuffer.h"
#include "../Core/ByteAddressBuffer.h"
#include "../Core/Scene.h"

struct SPBRInputBufferDesc
{
	CColorBuffer* pPosBuffer = nullptr;
	CColorBuffer* pNormBuffer = nullptr;
	CColorBuffer* pBaseColorBuffer = nullptr;
	CColorBuffer* pSpecularBuffer = nullptr;
	CColorBuffer* pShadowAndSSSBuffer = nullptr;
	bool ClearHint = true;

	SPBRInputBufferDesc()
	{
	}

	SPBRInputBufferDesc(CColorBuffer* vPosBuffer, CColorBuffer* vNormBuffer, CColorBuffer* vBaseColorBuffer, CColorBuffer* vSpecularBuffer, CColorBuffer* vShadowAndSSSBuffer = nullptr,  bool vClearHint = true)
		: pPosBuffer(vPosBuffer), pNormBuffer(vNormBuffer), pBaseColorBuffer(vBaseColorBuffer), pSpecularBuffer(vSpecularBuffer), pShadowAndSSSBuffer(vShadowAndSSSBuffer), ClearHint(vClearHint)
	{
	}
};

class CSDFDeferredShadingPBREffect
{
	enum EStaticSamplerRegister
	{
		kLinearWrapSamplerRegister = 0,

		nStaticSamplerNum
	};

	enum ERootParameter
	{
		kPositionMapSRV = 0,
		kNormMapSRV,
		kBaseColorMapSRV,
		kSpecularMapSRV,
		kShadowAndSSSMapSRV,
		kLightDataSRV,
		kLightIndecesSRV,

		kPerFrameCB,

		nRootParameterNum
	};

	enum ESRVRegister
	{
		kPositionMapSRVRegister = 0,
		kNormMapSRVRegister,
		kBaseColorMapSRVRegister,
		kSpecularMapSRVRegister,
		kShadowAndSSSMapSRVRegister,
		kLightDataSRVRegister,
		kLightIndecesSRVRegister
	};

	enum SceneRenderCBRegister
	{
		kPerFrameCBRegister = 0
	};

	struct SPerframe
	{
		XMMATRIX InvProj;
		XMFLOAT3 CameraPosW;
		UINT LightRenderCount;
		XMFLOAT3 AmbientLight;
		bool SDFEffectEnable = false;
	};

public:
	CSDFDeferredShadingPBREffect();
	~CSDFDeferredShadingPBREffect();

	void Init(CScene* pScene, DXGI_FORMAT PBROutputMapFormat, UINT PBRMapWidth, UINT PBRMapHeight, XMFLOAT3 AmbientLight);
	void Render(const std::wstring& cameraName, const SPBRInputBufferDesc* pGBufferOutputDesc, CColorBuffer* pOutputPBRMap, CGraphicsCommandList* pCommandList);

private:
	CScene* m_pScene;

	SPerframe m_CBPerFrame;

	CGraphicsPSO m_PSO;
	CRootSignature m_RootSignature;

	D3D12_VIEWPORT m_ViewPort;
	D3D12_RECT m_ScissorRect;

	DXGI_FORMAT m_PBROutputMapFormat;

	CStructuredBuffer m_QuadVertexBuffer;
	CByteAddressBuffer m_QuadIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_QuadVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_QuadIndexBufferView;

	CStructuredBuffer m_LightDataBuffer;
	CColorBuffer m_ShadowAndSSSBuffer;

	void __InitQuadGeomtry();
	void __BuildConstantBuffer();
	void __BuildRootSignature();
	void __BuildPSO();
};