#pragma once
#include "Scene.h"
#include "RenderCommon.h"
#include "RootSignature.h"
#include "CommandList.h"
#include "StructuredBuffer.h"
#include "ColorBuffer.h"

struct SSceneLightDeferredRenderConfig
{
	CScene* pScene = nullptr;
	UINT ActiveCameraIndex = INVALID_OBJECT_INDEX;
	UINT MaxSceneLightCount = 50;
	XMFLOAT4 AmbientLight = { 0.2f, 0.2f, 0.2f, 1.0f };

	UINT GBufferWidth = 0;
	UINT GBufferHeight = 0;
};

struct SDeferredRenderInputDesc
{
	CColorBuffer* pPosMap = nullptr;
	CColorBuffer* pNormMap = nullptr;
	CColorBuffer* pDiffMap = nullptr;
	CColorBuffer* pSpecMap = nullptr;
	CColorBuffer* pIdMap = nullptr;

	SDeferredRenderInputDesc() {}
	SDeferredRenderInputDesc(CColorBuffer* pos, CColorBuffer* norm, CColorBuffer* diff, CColorBuffer* spec, CColorBuffer* id)
		:pPosMap(pos), pNormMap(norm), pDiffMap(diff), pSpecMap(spec), pIdMap(id)
	{
	}
};

class CSceneLightDeferredRender
{
	enum ESceneRenderRootParameter
	{
		kGBufferSrvs = 0,
		kLightDataSrv,
		kLightRenderIndecesSrv,

		kPerFrameCB,

		nSceneRenderRootParameterNum
	};

	enum ESceneRenderSRVRegister
	{
		kGBufferSrvRegister = 0,
		kLightDataSrvRegister = 5,
		kLightRenderIndecesSrvRegister,
		kInstanceDataSrvRegister,
	};

	enum ESceneRenderCBRegister
	{
		kPerFrameCbRegister
	};

public:
	CSceneLightDeferredRender();
	~CSceneLightDeferredRender();

	void InitRender(const SSceneLightDeferredRenderConfig& renderConfig);

	UINT GetStaticSamplerCount() const { return 0; }
	UINT GetStaticSamplerStartRegister() const { return 0; }

	UINT GetRootParameterCount() const { return (UINT)nSceneRenderRootParameterNum; }
	UINT GetRootParameterStartIndex() const { return (UINT)(kGBufferSrvs); }

	void ConfigurateRootSignature(CRootSignature& rootSignature);

	void DrawQuad(const SDeferredRenderInputDesc& inputDesc, CGraphicsCommandList* pCommandList);


private:
	struct SFrameParameter
	{
		XMFLOAT4 EyePosW = { 0.0f, 0.0f, 0.0f, 1.0f };
		XMFLOAT4 AmbientLight = { 0.2f, 0.2f, 0.2f, 1.0f };
		UINT LightRenderCount = 0;
		UINT TexWidth = 0;
		UINT TexHeight = 0;
		UINT Pad;
	};

	SSceneLightDeferredRenderConfig m_RenderConfig;

	SFrameParameter m_FrameParameter;

	CStructuredBuffer m_LightDataBuffer;

	D3D12_VIEWPORT m_ViewPort;
	D3D12_RECT m_ScissorRect;

	CStructuredBuffer m_QuadVertexBuffer;
	CByteAddressBuffer m_QuadIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_QuadVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_QuadIndexBufferView;

	void __InitQuadGeomtry();
};

