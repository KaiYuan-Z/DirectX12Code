#include "SceneGeometryOnlyRender.h"
#include "GraphicsCommon.h"
#include "GraphicsCore.h"


CSceneGeometryOnlyRender::CSceneGeometryOnlyRender()
{
}

CSceneGeometryOnlyRender::~CSceneGeometryOnlyRender()
{
}

void CSceneGeometryOnlyRender::InitRender(const SSceneGeometryOnlyConfig& renderConfig)
{
	_ASSERTE(renderConfig.pScene);
	_ASSERTE(renderConfig.ActiveCameraIndex != INVALID_OBJECT_INDEX);
	UINT GrassInstCount = renderConfig.pScene->FetchGrassManager().GetGrarssPatchCount();
	m_RenderConfig = renderConfig;
	m_RenderConfig.MaxModelInstanceCount = max(m_RenderConfig.MaxModelInstanceCount, GrassInstCount);
	m_InstanceDataSet.resize(m_RenderConfig.MaxModelInstanceCount);
	m_PreInstanceWorldSet.resize(m_RenderConfig.MaxModelInstanceCount);
}

void CSceneGeometryOnlyRender::ConfigurateRootSignature(CRootSignature& rootSignature)
{
	_ASSERTE(rootSignature.GetRootParameterCount() >= nSceneRenderRootParameterNum);
	rootSignature.InitStaticSampler(kLinearSampler, 1, GraphicsCommon::Sampler::SamplerLinearWrapDesc);
	rootSignature[kInstanceDataSRV].InitAsBufferSRV(kInstanceDataSrvRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kPreInstanceWorldSRV].InitAsBufferSRV(kPreInstanceWorldSrvRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kDiffuseTextureSRV].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kDiffuseTextureSrvRegister, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
	rootSignature[kPerFrameCB].InitAsConstantBuffer(kPerFrameCbRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kMeshInfoCB].InitAsConstants(kMeshInfoCbRegister, 2, 1, D3D12_SHADER_VISIBILITY_ALL);
}

void CSceneGeometryOnlyRender::DrawScene(CGraphicsCommandList* pCommandList, bool useAlphaClip)
{
	_ASSERTE(pCommandList);

	CScene* pScene = m_RenderConfig.pScene;
	_ASSERTE(pScene);
	
	pScene->SetActiveCamera(m_RenderConfig.ActiveCameraIndex);
	auto& modelRenderListSet = pScene->GetActiveCameraModelRenderListSet().RenderListSet;

	CCameraBasic* pCamera = pScene->GetCamera(m_RenderConfig.ActiveCameraIndex);
	_ASSERTE(pCamera);
	m_FrameParameter.PreView = pCamera->GetPreViewXMM();
	m_FrameParameter.CurView = pCamera->GetViewXMM();
	m_FrameParameter.Proj = pCamera->GetProjectXMM();
	m_FrameParameter.CurViewProjWithOutJitter = pCamera->GetViewXMM() * pCamera->GetProjectXMM(false);
	m_FrameParameter.PreViewProjWithOutJitter = pCamera->GetPreViewXMM()*pCamera->GetProjectXMM(false);
	
	pCommandList->SetDynamicConstantBufferView(kPerFrameCB, sizeof(SFrameParameter), &m_FrameParameter);
	pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	__DrawSceneModels(modelRenderListSet, useAlphaClip, pCommandList);
	__DrawSceneGrass(pCommandList);
}

void CSceneGeometryOnlyRender::__DrawSceneModels(const std::vector<SModelRenderList>& renderListSet, bool useAlphaClip, CGraphicsCommandList* pCommandList)
{
	_ASSERTE(pCommandList);
	
	CScene* pScene = m_RenderConfig.pScene;
	_ASSERTE(pScene);
	
	UINT modelCount = (UINT)renderListSet.size();
	for (UINT sceneModelIndex = 0; sceneModelIndex < modelCount; sceneModelIndex++)
	{
		UINT instanceCount = (UINT)renderListSet[sceneModelIndex].InstanceIndexSet.size();
		instanceCount = instanceCount > m_RenderConfig.MaxModelInstanceCount ? m_RenderConfig.MaxModelInstanceCount : instanceCount;

		UINT finalInstanceCount = 0;
		for (UINT i = 0; i < instanceCount; i++)
		{
			UINT InstanceIndex = renderListSet[sceneModelIndex].InstanceIndexSet[i];
			CModelInstance* pInstance = pScene->GetInstance(InstanceIndex);
			_ASSERTE(pInstance);

			m_InstanceDataSet[finalInstanceCount].InstanceTag = (pInstance->IsDynamic()&& pInstance->IsUpdatedInCurrentFrame()) ? DYNAMIC_INSTANCE_TAG : 0;
			m_InstanceDataSet[finalInstanceCount].InstanceIndex = InstanceIndex;
			m_InstanceDataSet[finalInstanceCount].SceneModelIndex = sceneModelIndex;
			m_InstanceDataSet[finalInstanceCount].World = pInstance->GetTransformMatrix();
			m_PreInstanceWorldSet[finalInstanceCount] = pInstance->IsUpdatedInCurrentFrame() ? pInstance->GetPrevTransformMatrix() : pInstance->GetTransformMatrix();
			finalInstanceCount++;
		}

		if (finalInstanceCount > 0)
		{
			pCommandList->SetDynamicSRV(kInstanceDataSRV, finalInstanceCount * sizeof(SInstanceData), m_InstanceDataSet.data());
			pCommandList->SetDynamicSRV(kPreInstanceWorldSRV, finalInstanceCount * sizeof(XMMATRIX), m_PreInstanceWorldSet.data());

			CModel* pModel = pScene->GetModelInScene(renderListSet[sceneModelIndex].HostModelIndex);
			_ASSERTE(pModel);
			__DrawModelInstanced(pModel, finalInstanceCount, useAlphaClip, pCommandList);
		}
	}
}

void CSceneGeometryOnlyRender::__DrawModelInstanced(CModel* pModel, UINT instanceCount, bool useAlphaClip, CGraphicsCommandList* pCommandList)
{
	_ASSERTE(pModel && pCommandList);

	pCommandList->SetVertexBuffer(0, pModel->GetVertexBufferView());
	pCommandList->SetIndexBuffer(pModel->GetIndexBufferView());

	UINT indexDataTypeSize = pModel->GetIndexDataTypeSize();
	UINT vertexStride = pModel->GetVertexStride();

	for (UINT meshIndex = 0; meshIndex < pModel->GetHeader().meshCount; meshIndex++)
	{
		const CModel::SMesh& mesh = pModel->GetMesh(meshIndex);
		UINT materialIdx = 0xFFFFFFFFul;
		if (mesh.materialIndex != materialIdx)
		{
			materialIdx = mesh.materialIndex;
			pCommandList->SetDynamicDescriptor(kDiffuseTextureSRV, 0, pModel->GetSRVs(materialIdx)[CModel::kDiffuseOffset]);
		}

		pCommandList->SetConstants(kMeshInfoCB, meshIndex, useAlphaClip);

		UINT indexCount = mesh.indexCount;
		UINT startIndex = mesh.indexDataByteOffset / indexDataTypeSize;
		UINT baseVertex = mesh.vertexDataByteOffset / vertexStride;
		pCommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, 0);
	}
}

void CSceneGeometryOnlyRender::__DrawSceneGrass(CGraphicsCommandList* pCommandList)
{
	_ASSERTE(m_RenderConfig.pScene);
	auto& GrassManager = m_RenderConfig.pScene->FetchGrassManager();
	
	UINT instanceCount = GrassManager.GetGrarssPatchCount();
	instanceCount = instanceCount > m_RenderConfig.MaxModelInstanceCount ? m_RenderConfig.MaxModelInstanceCount : instanceCount;

	UINT instanceOffset = m_RenderConfig.pScene->GetInstanceCount();
	UINT modelOffset = m_RenderConfig.pScene->GetModelCountInScene();

	UINT finalInstanceCount = 0;
	for (UINT i = 0; i < instanceCount; i++)
	{
		m_InstanceDataSet[finalInstanceCount].InstanceTag = DEFORMABLE_INSTANCE_TAG | GRASS_INST_TAG | DYNAMIC_INSTANCE_TAG;
		m_InstanceDataSet[finalInstanceCount].InstanceIndex = instanceOffset + i;
		m_InstanceDataSet[finalInstanceCount].SceneModelIndex = modelOffset + 0;
		m_InstanceDataSet[finalInstanceCount].World = GrassManager.GetGrarssPatchTransform(i);
		m_PreInstanceWorldSet[finalInstanceCount] = GrassManager.GetGrarssPatchTransform(i);
		finalInstanceCount++;
	}

	pCommandList->SetDynamicSRV(kInstanceDataSRV, finalInstanceCount * sizeof(SInstanceData), m_InstanceDataSet.data());
	pCommandList->SetDynamicSRV(kPreInstanceWorldSRV, finalInstanceCount * sizeof(XMMATRIX), m_PreInstanceWorldSet.data());

	UINT indexDataTypeSize = GrassManager.c_IndexTypeSize;
	UINT vertexStride = GrassManager.c_VertexStride;

	UINT meshIndex = 0;
	bool useAlphaClip = false;
	pCommandList->SetConstants(kMeshInfoCB, meshIndex, useAlphaClip);

	pCommandList->SetVertexBuffer(0, GrassManager.GetGrarssPatchVertexBufferView());
	pCommandList->SetIndexBuffer(GrassManager.GetGrarssPatchIndexBufferView());

	UINT indexCount = GrassManager.FetchGrarssPatchIndexBuffer().GetElementCount();
	UINT startIndex = 0;
	UINT baseVertex = 0;
	pCommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, 0);
}
