#pragma once
#include "GraphicsCoreBase.h"
#include "Scene.h"
#include "RenderCommon.h"
#include "RootSignature.h"
#include "CommandList.h"
#include "TripleUploadBuffer.h"
#include "SDFCommon.h"

struct SSDFSceneRenderConfig
{
	CScene* pScene = nullptr;
	UINT ActiveCameraIndex = INVALID_OBJECT_INDEX;
	UINT MaxInstanceCount = 100;
	UINT MaxMeshSDFCount = 50;
	std::vector<UINT> SDFLightIndexSet;
	UINT SSSLightIndex = INVALID_OBJECT_INDEX;
	XMFLOAT4 AmbientLight = { 0.2f, 0.2f, 0.2f, 1.0f };
	CColorBuffer* pPositionMap = nullptr;
	CColorBuffer* pNormMap = nullptr;
	CColorBuffer* pModelIDAndInstanceIDMap = nullptr;
};

class CSDFSceneRender
{
	enum ESceneRenderStaticSamplerRegister
	{
		kLinearWrapSamplerRegister = 0,

		nSceneRenderStaticSamplerNum
	};

	enum ESceneRenderRootParameter
	{
		kPositionMapSRV = 0,
		kNormMapSRV,
		kModelIDAndInstanceIDMapSRV,
		kLightDataSRV,
		kModelDataSRV,
		kInstanceDataSRV,
		kMeshSDFSRVs,

		kPerFrameCB,
		kSceneCB,

		nSceneRenderRootParameterNum
	};

	enum ESceneRenderSRVRegister
	{
		kPositionMapSRVRegister = 0,
		kNormMapSRVRegister,
		kModelIDAndInstanceIDMapSRVRegister,
		kLightDataSRVRegister,
		kModelDataSRVRegister,
		kInstanceDataSRVRegister,
		kMeshSDFSRVsRegister
	};

	enum ESceneRenderCBRegister
	{
		kPerFrameCBRegister = 0,
		kSceneCBRegister
	};

	struct SPerframe
	{
		XMFLOAT4 CameraPosW;
		XMMATRIX View;
		XMMATRIX Proj;
		XMMATRIX InvProj;
		XMFLOAT4 AmbientLight;
	};

	struct SSceneCB
	{
		UINT SDFLightCount;
		UINT SSSLightIndex;
		UINT SDFInstanceNum;
	};

public:
	CSDFSceneRender();
	~CSDFSceneRender();

	void InitRender(const SSDFSceneRenderConfig& renderConfig);

	UINT GetStaticSamplerCount() const { return (UINT)nSceneRenderStaticSamplerNum; }
	UINT GetStaticSamplerStartRegister() const { return (UINT)(kLinearWrapSamplerRegister); }

	UINT GetRootParameterCount() const { return (UINT)nSceneRenderRootParameterNum; }
	UINT GetRootParameterStartIndex() const { return (UINT)(kPositionMapSRV); }

	void ConfigurateRootSignature(CRootSignature& rootSignature);

	void DrawQuad(CGraphicsCommandList* pCommandList);

private:
	SSDFSceneRenderConfig m_RenderConfig;
	CScene* m_pScene;
	SPerframe m_CBPerframe;
	SSceneCB m_CBScene;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_SDFSrvSet;
	std::vector<SModelSDFData> m_ModelDataSet;
	std::vector<SInstanceData> m_InstanceDataSet;

	CColorBuffer* m_pPositionMap;
	CColorBuffer* m_pNormMap;
	CColorBuffer* m_pModelIDAndInstanceIDMap;
	CStructuredBuffer			m_QuadVertexBuffer;
	CByteAddressBuffer			m_QuadIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	m_QuadVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW		m_QuadIndexBufferView;

	CStructuredBuffer m_SceneConstantBuffer;
	CStructuredBuffer m_SDFLightDataBuffer;
	CStructuredBuffer m_ModelDataBuffer;
	CTripleUploadBuffer m_InstanceDataUploadBuffer;

	void __InitQuadGeomtry();
	void __UpdateBuffer();
};
