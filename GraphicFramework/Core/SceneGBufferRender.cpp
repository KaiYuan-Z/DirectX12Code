#include "SceneGBufferRender.h"
#include "GraphicsCommon.h"
#include "GraphicsCore.h"


CSceneGBufferRender::CSceneGBufferRender()
{
}

CSceneGBufferRender::~CSceneGBufferRender()
{
}

void CSceneGBufferRender::InitRender(const SGBufferRenderConfig& renderConfig)
{
	_ASSERTE(renderConfig.pScene);
	_ASSERTE(renderConfig.ActiveCameraIndex != INVALID_OBJECT_INDEX);
	m_RenderConfig = renderConfig;

	m_FrameParameter.EnableNormalMap = renderConfig.EnableNormalMap;
	m_FrameParameter.EnableNormalMapCompression = renderConfig.EnableNormalMapCompression;

	m_InstanceDataSet.resize(renderConfig.MaxModelInstanceCount);
	m_PreInstanceWorldSet.resize(renderConfig.MaxModelInstanceCount);

	CScene* pScene = renderConfig.pScene;
}

void CSceneGBufferRender::ConfigurateRootSignature(CRootSignature& rootSignature)
{
	_ASSERTE(rootSignature.GetStaticSamplerCount() >= nSceneRenderStaticSamplerNum);
	rootSignature.InitStaticSampler(kLinearWrapSamplerRegister, 1, GraphicsCommon::Sampler::SamplerLinearWrapDesc, D3D12_SHADER_VISIBILITY_ALL);

	_ASSERTE(rootSignature.GetRootParameterCount() >= nSceneRenderRootParameterNum);
	rootSignature[kMeshTexSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kMeshTexSrvRegister, 6, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kInstanceDataSRV].InitAsBufferSRV(kInstanceDataSrvRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kPreInstanceWorldSRV].InitAsBufferSRV(kPreInstanceWorldSrvRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kPerFrameCB].InitAsConstantBuffer(kPerFrameCbRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kMeshMaterialCB].InitAsConstantBuffer(kMeshMaterialCbRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kMeshIndexCB].InitAsConstants(kMeshIndexCbRegister, 1, 1, D3D12_SHADER_VISIBILITY_ALL);
}

void CSceneGBufferRender::DrawScene(CGraphicsCommandList* pCommandList)
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
	m_FrameParameter.PreViewProjWithOutJitter = pCamera->GetPreViewXMM() * pCamera->GetProjectXMM(false);
	
	pCommandList->SetDynamicConstantBufferView(kPerFrameCB, sizeof(SFrameParameter), &m_FrameParameter);
	pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	__DrawSceneModels(pCommandList, modelRenderListSet);
	__DrawSceneGrass(pCommandList);
}

void CSceneGBufferRender::__DrawSceneModels(CGraphicsCommandList* pCommandList, const std::vector<SModelRenderList>& renderListSet)
{
	_ASSERTE(pCommandList);
	
	CScene* pScene = m_RenderConfig.pScene;
	_ASSERTE(pScene);
	
	UINT modelCount = (UINT)renderListSet.size();
	for (UINT sceneModelIndex = 0; sceneModelIndex < modelCount; sceneModelIndex++)
	{
		UINT instanceCount = (UINT)renderListSet[sceneModelIndex].InstanceIndexSet.size();
		instanceCount = instanceCount > m_RenderConfig.MaxModelInstanceCount ? m_RenderConfig.MaxModelInstanceCount : instanceCount;

		for (UINT i = 0; i < instanceCount; i++)
		{
			UINT InstanceIndex = renderListSet[sceneModelIndex].InstanceIndexSet[i];
			CModelInstance* pInstance = pScene->GetInstance(InstanceIndex);
			_ASSERTE(pInstance);

			m_InstanceDataSet[i].InstanceTag = pInstance->IsDynamic() ? DYNAMIC_INSTANCE_TAG : 0;
			m_InstanceDataSet[i].InstanceIndex = InstanceIndex;
			m_InstanceDataSet[i].SceneModelIndex = sceneModelIndex;
			m_InstanceDataSet[i].World = pInstance->GetTransformMatrix();
			m_PreInstanceWorldSet[i] = pInstance->IsUpdatedInCurrentFrame() ? pInstance->GetPrevTransformMatrix() : pInstance->GetTransformMatrix();
		}
		pCommandList->SetDynamicSRV(kInstanceDataSRV, instanceCount * sizeof(SInstanceData), m_InstanceDataSet.data());
		pCommandList->SetDynamicSRV(kPreInstanceWorldSRV, instanceCount * sizeof(XMMATRIX), m_PreInstanceWorldSet.data());

		CModel* pModel = pScene->GetModelInScene(renderListSet[sceneModelIndex].HostModelIndex);
		_ASSERTE(pModel);
		__DrawModelInstanced(pModel, instanceCount, pCommandList);
	}
}

void CSceneGBufferRender::__DrawModelInstanced(CModel* pModel, UINT instanceCount, CGraphicsCommandList* pCommandList)
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
			pCommandList->SetDynamicDescriptors(kMeshTexSRVs, 0, 6, pModel->GetSRVs(materialIdx));

			//const CModel::SMaterial& material = pModel->GetMaterial(materialIdx);
			//const XMFLOAT3& DiffuseAlbedo = material.diffuse;
			//m_MeshMaterial.DiffuseAlbedo = { DiffuseAlbedo.x, DiffuseAlbedo.y, DiffuseAlbedo.z, 1.0 };
			//m_MeshMaterial.Shininess = material.shininess;
			//m_MeshMaterial.FresnelR0 = material.specular;
			pCommandList->SetDynamicConstantBufferView(kMeshMaterialCB, sizeof(SMeshMaterial), &m_MeshMaterial);
		}

		pCommandList->SetConstant(kMeshIndexCB, meshIndex, 0);

		UINT indexCount = mesh.indexCount;
		UINT startIndex = mesh.indexDataByteOffset / indexDataTypeSize;
		UINT baseVertex = mesh.vertexDataByteOffset / vertexStride;
		pCommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, 0);
	}
}

void CSceneGBufferRender::__DrawSceneGrass(CGraphicsCommandList* pCommandList)
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
	pCommandList->SetConstant(kMeshIndexCB, meshIndex);
	pCommandList->SetDynamicSRV(kInstanceDataSRV, finalInstanceCount * sizeof(SInstanceData), m_InstanceDataSet.data());
	pCommandList->SetDynamicConstantBufferView(kMeshMaterialCB, sizeof(SMeshMaterial), &GrassManager.FetchGrassPatchMaterial());
	pCommandList->SetDynamicDescriptors(kMeshTexSRVs, 0, 6, GrassManager.GetGrarssPatchTextureSRVs());

	pCommandList->SetVertexBuffer(0, GrassManager.GetGrarssPatchVertexBufferView());
	pCommandList->SetIndexBuffer(GrassManager.GetGrarssPatchIndexBufferView());

	UINT indexCount = GrassManager.FetchGrarssPatchIndexBuffer().GetElementCount();
	UINT startIndex = 0;
	UINT baseVertex = 0;
	pCommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, 0);
}
