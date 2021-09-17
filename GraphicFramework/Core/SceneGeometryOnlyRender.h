#pragma once
#include "Scene.h"
#include "RenderCommon.h"
#include "RootSignature.h"
#include "CommandList.h"
#include "StructuredBuffer.h"

struct SSceneGeometryOnlyConfig
{
	CScene* pScene = nullptr;
	UINT ActiveCameraIndex = INVALID_OBJECT_INDEX;
	UINT MaxModelInstanceCount = 50;
};

class CSceneGeometryOnlyRender
{
	enum ESceneRenderRootParameter
	{
		kInstanceDataSRV = 0,
		kPreInstanceWorldSRV,
		kDiffuseTextureSRV,

		kPerFrameCB,
		kMeshInfoCB,

		nSceneRenderRootParameterNum
	};

	enum ESceneRenderSRVRegister
	{
		kInstanceDataSrvRegister,
		kPreInstanceWorldSrvRegister,
		kDiffuseTextureSrvRegister
	};

	enum ESceneRenderCBRegister
	{
		kPerFrameCbRegister = 0,
		kMeshInfoCbRegister
	};

	enum ESceneRenderStaticSampler
	{
		kLinearSampler = 0,

		kStaticSamplerNum
	};

public:
	CSceneGeometryOnlyRender();
	~CSceneGeometryOnlyRender();

	void InitRender(const SSceneGeometryOnlyConfig& renderConfig);

	UINT GetStaticSamplerCount() const { return (UINT)kStaticSamplerNum; }
	UINT GetStaticSamplerStartRegister() const { return (UINT)kLinearSampler; }

	UINT GetRootParameterCount() const { return (UINT)nSceneRenderRootParameterNum; }
	UINT GetRootParameterStartIndex() const { return (UINT)(kInstanceDataSRV); }

	void ConfigurateRootSignature(CRootSignature& rootSignature);

	void DrawScene(CGraphicsCommandList* pCommandList, bool useAlphaClip = true);


private:
	struct SFrameParameter
	{
		XMMATRIX PreView = XMMatrixIdentity();
		XMMATRIX CurView = XMMatrixIdentity();
		XMMATRIX Proj = XMMatrixIdentity();
		XMMATRIX CurViewProjWithOutJitter = XMMatrixIdentity();
		XMMATRIX PreViewProjWithOutJitter = XMMatrixIdentity();
	};

	SSceneGeometryOnlyConfig m_RenderConfig;

	SFrameParameter m_FrameParameter;
	std::vector<SInstanceData> m_InstanceDataSet;
	std::vector<XMMATRIX> m_PreInstanceWorldSet;

	void __DrawSceneModels(const std::vector<SModelRenderList>& renderListSet, bool useAlphaClip, CGraphicsCommandList* pCommandList);
	void __DrawModelInstanced(CModel* pModel, UINT instanceCount, bool useAlphaClip,CGraphicsCommandList* pCommandList);
	void __DrawSceneGrass(CGraphicsCommandList* pCommandList);
};

