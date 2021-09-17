#pragma once
#include "Scene.h"
#include "RenderCommon.h"
#include "UploadBuffer.h"
#include "RootSignature.h"
#include "CommandList.h"
#include "StructuredBuffer.h"

struct SGBufferRenderConfig
{
	CScene* pScene = nullptr;
	UINT ActiveCameraIndex = INVALID_OBJECT_INDEX;
	UINT MaxModelInstanceCount = 50;
	bool EnableNormalMap = false;
	bool EnableNormalMapCompression = false;
};

class CSceneGBufferRender
{
	enum ESceneRenderStaticSamplerRegister
	{
		kLinearWrapSamplerRegister = 0,

		nSceneRenderStaticSamplerNum
	};

	enum ESceneRenderRootParameter
	{
		kMeshTexSRVs = 0,
		kInstanceDataSRV,
		kPreInstanceWorldSRV,

		kPerFrameCB,
		kMeshMaterialCB,
		kMeshIndexCB,

		nSceneRenderRootParameterNum
	};

	enum ESceneRenderSRVRegister
	{
		kMeshTexSrvRegister = 0,
		kInstanceDataSrvRegister = 6,
		kPreInstanceWorldSrvRegister,
	};

	enum ESceneRenderCBRegister
	{
		kPerFrameCbRegister = 0,
		kMeshMaterialCbRegister,
		kMeshIndexCbRegister
	};


public:
	CSceneGBufferRender();
	~CSceneGBufferRender();

	void InitRender(const SGBufferRenderConfig& renderConfig);

	UINT GetStaticSamplerCount() const { return (UINT)nSceneRenderStaticSamplerNum; }
	UINT GetStaticSamplerStartRegister() const { return (UINT)(kLinearWrapSamplerRegister); }

	UINT GetRootParameterCount() const { return (UINT)nSceneRenderRootParameterNum; }
	UINT GetRootParameterStartIndex() const { return (UINT)(kMeshTexSRVs); }

	void ConfigurateRootSignature(CRootSignature& rootSignature);

	void DrawScene(CGraphicsCommandList* pCommandList);


private:
	struct SFrameParameter
	{
		XMMATRIX PreView = XMMatrixIdentity();
		XMMATRIX CurView = XMMatrixIdentity();
		XMMATRIX Proj = XMMatrixIdentity();
		XMMATRIX CurViewProjWithOutJitter = XMMatrixIdentity();
		XMMATRIX PreViewProjWithOutJitter = XMMatrixIdentity();
		UINT EnableNormalMap = 0;
		UINT EnableNormalMapCompression = 0;
		XMUINT2 Pad;
	};

	SGBufferRenderConfig m_RenderConfig;

	SMeshMaterial m_MeshMaterial;
	SFrameParameter m_FrameParameter;
	std::vector<SInstanceData> m_InstanceDataSet;
	std::vector<XMMATRIX> m_PreInstanceWorldSet;

	void __DrawSceneModels(CGraphicsCommandList* pCommandList, const std::vector<SModelRenderList>& renderListSet);
	void __DrawModelInstanced(CModel* pModel, UINT instanceCount, CGraphicsCommandList* pCommandList);
	void __DrawSceneGrass(CGraphicsCommandList* pCommandList);
};

