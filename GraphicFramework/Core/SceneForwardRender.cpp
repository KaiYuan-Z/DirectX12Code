#include "SceneForwardRender.h"
#include "GraphicsCommon.h"


CSceneForwardRender::CSceneForwardRender()
{
}

CSceneForwardRender::~CSceneForwardRender()
{
}

void CSceneForwardRender::InitRender(const SSceneForwardRenderConfig& renderConfig)
{
	_ASSERTE(renderConfig.pScene);
	_ASSERTE(renderConfig.ActiveCameraIndex != INVALID_OBJECT_INDEX);
	m_RenderConfig = renderConfig;

	m_FrameParameter.AmbientLight = renderConfig.AmbientLight;
	m_FrameParameter.EnableNormalMap = renderConfig.EnableNormalMap;
	m_FrameParameter.EnableNormalMapCompression = renderConfig.EnableNormalMapCompression;

	m_InstanceDataSet.resize(renderConfig.MaxModelInstanceCount);

	CScene* pScene = renderConfig.pScene;
	UINT lightCount = pScene->GetLightCount();
	lightCount = lightCount > renderConfig.MaxSceneLightCount ? renderConfig.MaxSceneLightCount : lightCount;
	std::vector<SLightData> lightDataSet(lightCount);
	for (UINT i = 0; i < lightCount; i++)
	{
		CLight* pLight = pScene->GetLight(i);
		_ASSERTE(pLight);
		lightDataSet[i] = pLight->GetLightData();
	}
	m_LightDataBuffer.Create(L"LightData", lightCount, sizeof(SLightData), lightDataSet.data());
}

void CSceneForwardRender::ConfigurateRootSignature(CRootSignature& rootSignature)
{
	_ASSERTE(rootSignature.GetStaticSamplerCount() >= nSceneRenderStaticSamplerNum);
	rootSignature.InitStaticSampler(kLinearWrapSamplerRegister, 1, GraphicsCommon::Sampler::SamplerLinearWrapDesc, D3D12_SHADER_VISIBILITY_ALL);

	_ASSERTE(rootSignature.GetRootParameterCount() >= nSceneRenderRootParameterNum);
	rootSignature[kMeshTexSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kMeshTexSrvRegister, 6, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kLightDataSRV].InitAsBufferSRV(kLightDataSrvRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kLightRenderIndecesSRV].InitAsBufferSRV(kLightRenderIndecesSrvRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kInstanceDataSRV].InitAsBufferSRV(kInstanceDataSrvRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kPerFrameCB].InitAsConstantBuffer(kPerFrameCbRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kMeshMaterialCB].InitAsConstantBuffer(kMeshMaterialCbRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
}

void CSceneForwardRender::DrawScene(CGraphicsCommandList* pCommandList)
{
	_ASSERTE(pCommandList);

	CScene* pScene = m_RenderConfig.pScene;
	_ASSERTE(pScene);
	
	pScene->SetActiveCamera(m_RenderConfig.ActiveCameraIndex);
	auto& modelRenderListSet = pScene->GetActiveCameraModelRenderListSet().RenderListSet;
	auto& lightRenderSet = pScene->GetActiveCameraLightRenderSet().LightIndexSet;

	CCameraBasic* pCamera = pScene->GetCamera(m_RenderConfig.ActiveCameraIndex);
	_ASSERTE(pCamera);
	m_FrameParameter.ViewProj = pCamera->GetViewXMM()*pCamera->GetProjectXMM();
	XMFLOAT3 CamPos = pCamera->GetPositionXMF3();
	m_FrameParameter.EyePosW = { CamPos.x, CamPos.y, CamPos.z, 1.0f };
	m_FrameParameter.LightRenderCount = (UINT)lightRenderSet.size();
	
	pCommandList->SetBufferSRV(kLightDataSRV, m_LightDataBuffer);
	pCommandList->SetDynamicSRV(kLightRenderIndecesSRV, lightRenderSet.size()*sizeof(UINT), lightRenderSet.data());
	pCommandList->SetDynamicConstantBufferView(kPerFrameCB, sizeof(SFrameParameter), &m_FrameParameter);
	pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	__DrawSceneModels(pCommandList, modelRenderListSet);
	__DrawSceneGrass(pCommandList);
}

void CSceneForwardRender::__DrawSceneModels(CGraphicsCommandList* pCommandList, const std::vector<SModelRenderList>& renderListSet)
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
		}
		pCommandList->SetDynamicSRV(kInstanceDataSRV, instanceCount * sizeof(SInstanceData), m_InstanceDataSet.data());

		CModel* pModel = pScene->GetModelInScene(renderListSet[sceneModelIndex].HostModelIndex);
		_ASSERTE(pModel);
		__DrawModelInstanced(pModel, instanceCount, pCommandList);
	}
}

void CSceneForwardRender::__DrawModelInstanced(CModel* pModel, UINT instanceCount, CGraphicsCommandList* pCommandList)
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

		UINT indexCount = mesh.indexCount;
		UINT startIndex = mesh.indexDataByteOffset / indexDataTypeSize;
		UINT baseVertex = mesh.vertexDataByteOffset / vertexStride;
		pCommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, 0);
	}
}

void CSceneForwardRender::__DrawSceneGrass(CGraphicsCommandList* pCommandList)
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
		finalInstanceCount++;
	}

	pCommandList->SetDynamicSRV(kInstanceDataSRV, finalInstanceCount * sizeof(SInstanceData), m_InstanceDataSet.data());
	pCommandList->SetDynamicConstantBufferView(kMeshMaterialCB, sizeof(SMeshMaterial), &GrassManager.FetchGrassPatchMaterial());
	pCommandList->SetDynamicDescriptors(kMeshTexSRVs, 0, 6, GrassManager.GetGrarssPatchTextureSRVs());

	UINT indexDataTypeSize = GrassManager.c_IndexTypeSize;
	UINT vertexStride = GrassManager.c_VertexStride;

	pCommandList->SetVertexBuffer(0, GrassManager.GetGrarssPatchVertexBufferView());
	pCommandList->SetIndexBuffer(GrassManager.GetGrarssPatchIndexBufferView());

	UINT indexCount = GrassManager.FetchGrarssPatchIndexBuffer().GetElementCount();
	UINT startIndex = 0;
	UINT baseVertex = 0;
	pCommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, 0);
}
