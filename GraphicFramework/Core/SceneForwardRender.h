#pragma once
#include "Scene.h"
#include "RenderCommon.h"
#include "RootSignature.h"
#include "CommandList.h"
#include "StructuredBuffer.h"

struct SSceneForwardRenderConfig
{
	CScene* pScene = nullptr;
	UINT ActiveCameraIndex = INVALID_OBJECT_INDEX;
	UINT MaxModelInstanceCount = 50;
	UINT MaxSceneLightCount = 50;
	bool EnableNormalMap = false;
	bool EnableNormalMapCompression = false;
	XMFLOAT4 AmbientLight = { 0.2f, 0.2f, 0.2f, 1.0f };
};

class CSceneForwardRender
{
	enum ESceneRenderStaticSamplerRegister
	{
		kLinearWrapSamplerRegister = 0,

		nSceneRenderStaticSamplerNum
	};

	enum ESceneRenderRootParameter
	{
		kMeshTexSRVs = 0,
		kLightDataSRV,
		kLightRenderIndecesSRV,
		kInstanceDataSRV,

		kPerFrameCB,
		kMeshMaterialCB, 

		nSceneRenderRootParameterNum
	};

	enum ESceneRenderSRVRegister
	{
		kMeshTexSrvRegister = 0,
		kLightDataSrvRegister = 6,
		kLightRenderIndecesSrvRegister,
		kInstanceDataSrvRegister,
	};

	enum ESceneRenderCBRegister
	{
		kPerFrameCbRegister = 0,
		kMeshMaterialCbRegister,
	};

public:
	CSceneForwardRender();
	~CSceneForwardRender();

	void InitRender(const SSceneForwardRenderConfig& renderConfig);

	UINT GetStaticSamplerCount() const { return (UINT)nSceneRenderStaticSamplerNum; }
	UINT GetStaticSamplerStartRegister() const { return (UINT)(kLinearWrapSamplerRegister); }

	UINT GetRootParameterCount() const { return (UINT)nSceneRenderRootParameterNum; }
	UINT GetRootParameterStartIndex() const { return (UINT)(kMeshTexSRVs); }

	void ConfigurateRootSignature(CRootSignature& rootSignature);

	void DrawScene(CGraphicsCommandList* pCommandList);


private:
	struct SFrameParameter
	{
		XMMATRIX ViewProj = XMMatrixIdentity();
		XMFLOAT4 EyePosW = { 0.0f, 0.0f, 0.0f, 1.0f };
		XMFLOAT4 AmbientLight = { 0.2f, 0.2f, 0.2f, 1.0f };
		UINT LightRenderCount = 0;
		UINT Pad;
		UINT EnableNormalMap = 0;
		UINT EnableNormalMapCompression = 0;
	};

	SSceneForwardRenderConfig m_RenderConfig;

	SMeshMaterial m_MeshMaterial;
	SFrameParameter m_FrameParameter;
	std::vector<SInstanceData> m_InstanceDataSet;

	CStructuredBuffer m_LightDataBuffer;

	void __DrawSceneModels(CGraphicsCommandList* pCommandList, const std::vector<SModelRenderList>& renderListSet);
	void __DrawModelInstanced(CModel* pModel, UINT instanceCount, CGraphicsCommandList* pCommandList); 
	void __DrawSceneGrass(CGraphicsCommandList* pCommandList);
};

