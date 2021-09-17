#include "SDFSceneRender.h"
#include "ModelSDF.h"

#define INVALID_SDF_INDEX 0xffff

CSDFSceneRender::CSDFSceneRender()
{
}

CSDFSceneRender::~CSDFSceneRender()
{
}

void CSDFSceneRender::InitRender(const SSDFSceneRenderConfig& renderConfig)
{
	_ASSERTE(renderConfig.pScene && renderConfig.pPositionMap && renderConfig.pModelIDAndInstanceIDMap && renderConfig.pNormMap);
	_ASSERTE(renderConfig.ActiveCameraIndex != INVALID_OBJECT_INDEX && renderConfig.SSSLightIndex != INVALID_OBJECT_INDEX);
	_ASSERTE(renderConfig.SDFLightIndexSet.size() > 0);
	m_RenderConfig = renderConfig;

	m_pScene = renderConfig.pScene;
	m_pPositionMap = renderConfig.pPositionMap;
	m_pNormMap = renderConfig.pNormMap;
	m_pModelIDAndInstanceIDMap = renderConfig.pModelIDAndInstanceIDMap;
	_ASSERTE(m_pScene && m_pPositionMap && m_pNormMap && m_pModelIDAndInstanceIDMap);

	m_CBPerframe.AmbientLight = renderConfig.AmbientLight;
	m_InstanceDataSet.resize(m_pScene->GetInstanceCount() > renderConfig.MaxInstanceCount ? renderConfig.MaxInstanceCount : m_pScene->GetInstanceCount());

	UINT modelCount = (UINT)m_pScene->GetModelCountInScene();
	m_ModelDataSet.resize(modelCount);
	m_CBScene.SDFInstanceNum = 0;

	for (UINT sceneModelIndex = 0; sceneModelIndex < modelCount; sceneModelIndex++)
	{
		UINT instanceCount = (UINT)m_pScene->GetModelInstanceCount(sceneModelIndex);
		CModel* Model = m_pScene->GetModelInScene(sceneModelIndex);

		m_ModelDataSet[sceneModelIndex].SDFParam.SDFIndex = INVALID_SDF_INDEX;

		if (Model->GetModelType() == MODEL_TYPE_SDF)
		{
			CModelSDF* ModelSDF = Model->QuerySdfModel();
			m_SDFSrvSet.push_back(ModelSDF->GetSDFBuffer().GetSRV());
			m_ModelDataSet[sceneModelIndex].IsUsingSSS = ModelSDF->QuerySSSUsingState();
			m_ModelDataSet[sceneModelIndex].SSSParam = ModelSDF->GetSSSParam();
			m_ModelDataSet[sceneModelIndex].SDFParam = ModelSDF->GetSDFParam();
			m_ModelDataSet[sceneModelIndex].SDFParam.SDFIndex = UINT(m_SDFSrvSet.size() - 1);
		}

		for (UINT i = 0; i < instanceCount; i++)
		{
			CModelInstance* pInstance = m_pScene->GetModelInstance(sceneModelIndex, i);
			_ASSERTE(pInstance);

			if (m_CBScene.SDFInstanceNum + i > m_InstanceDataSet.size() - 1)
				break;

			m_InstanceDataSet[i].InstanceTag = pInstance->IsDynamic() ? DYNAMIC_INSTANCE_TAG : 0;
			m_InstanceDataSet[m_CBScene.SDFInstanceNum + i].InstanceIndex = pInstance->GetIndex();
			m_InstanceDataSet[m_CBScene.SDFInstanceNum + i].SceneModelIndex = sceneModelIndex;
			m_InstanceDataSet[m_CBScene.SDFInstanceNum + i].World = XMMatrixInverse(nullptr, XMMatrixTranslationFromVector(XMLoadFloat3(&Model->GetBoundingObject().boundingBox.Center)) * pInstance->GetTransformMatrix());
			m_CBScene.SDFInstanceNum++;
		}
	}
	m_ModelDataBuffer.Create(L"ModelData", modelCount, sizeof(SModelSDFData), m_ModelDataSet.data());
	m_InstanceDataUploadBuffer.Create(L"InstanceData", m_CBScene.SDFInstanceNum, sizeof(SInstanceData), m_InstanceDataSet.data());

	m_CBScene.SDFLightCount = (UINT)renderConfig.SDFLightIndexSet.size();
	std::vector<SLightData> SDFLightDataSet(m_CBScene.SDFLightCount);
	for (UINT i = 0; i < m_CBScene.SDFLightCount; i++)
	{
		CLight* pLight = m_pScene->GetLight(renderConfig.SDFLightIndexSet[i]);
		_ASSERTE(pLight);
		SDFLightDataSet[i] = pLight->GetLightData();
		if (renderConfig.SDFLightIndexSet[i] == renderConfig.SSSLightIndex)
			m_CBScene.SSSLightIndex = i;
	}
	m_SDFLightDataBuffer.Create(L"LightData", m_CBScene.SDFLightCount, sizeof(SLightData), SDFLightDataSet.data());
	m_SceneConstantBuffer.Create(L"Scene", 1, sizeof(SSceneCB), &m_CBScene);

	__InitQuadGeomtry();
}

void CSDFSceneRender::ConfigurateRootSignature(CRootSignature& rootSignature)
{
	_ASSERTE(rootSignature.GetStaticSamplerCount() >= nSceneRenderStaticSamplerNum);
	rootSignature.InitStaticSampler(kLinearWrapSamplerRegister, 1, GraphicsCommon::Sampler::SamplerLinearWrapDesc, D3D12_SHADER_VISIBILITY_ALL);

	_ASSERTE(rootSignature.GetRootParameterCount() >= nSceneRenderRootParameterNum);
	rootSignature[kPositionMapSRV].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kPositionMapSRVRegister, 1, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kNormMapSRV].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kNormMapSRVRegister, 1, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kModelIDAndInstanceIDMapSRV].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kModelIDAndInstanceIDMapSRVRegister, 1, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kLightDataSRV].InitAsBufferSRV(kLightDataSRVRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kModelDataSRV].InitAsBufferSRV(kModelDataSRVRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kInstanceDataSRV].InitAsBufferSRV(kInstanceDataSRVRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kMeshSDFSRVs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kMeshSDFSRVsRegister, 20, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kPerFrameCB].InitAsConstantBuffer(kPerFrameCBRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kSceneCB].InitAsConstantBuffer(kSceneCBRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
}

void CSDFSceneRender::__InitQuadGeomtry()
{
	std::vector<GraphicsCommon::Vertex::SPosTex> QuadVertex = {
	{{-1.0f, -1.0f, 0.0f},{0.0f, 1.0f}},
	{{-1.0f,  1.0f, 0.0f},{0.0f, 0.0f}},
	{{1.0f,  1.0f, 0.0f},{1.0f, 0.0f}},
	{{1.0f, -1.0f, 0.0f},{1.0f, 1.0f}}
	};
	m_QuadVertexBuffer.Create(L"Quad Vertex Buffer", 4, sizeof(GraphicsCommon::Vertex::SPosTex), QuadVertex.data());
	m_QuadVertexBufferView = m_QuadVertexBuffer.CreateVertexBufferView();

	std::vector<uint16_t> QuadIndex = { 0, 1, 2, 0, 2, 3 };
	m_QuadIndexBuffer.Create(L"Quad Index Buffer", 6, sizeof(uint16_t), QuadIndex.data());
	m_QuadIndexBufferView = m_QuadIndexBuffer.CreateIndexBufferView();
}

void CSDFSceneRender::__UpdateBuffer()
{
	m_InstanceDataUploadBuffer.MoveToNextBuffer();
	SInstanceData* InstanceDataMap = (SInstanceData*)m_InstanceDataUploadBuffer.GetCurrentBufferMappedData();
	_ASSERTE(InstanceDataMap);
	const SInstanceSet& DirtyInstanceSet = m_pScene->GetRecentDirtyInstanceSet();
	if (DirtyInstanceSet.FrameID == GraphicsCore::GetFrameID())
	{
		for (auto i = 0; i < DirtyInstanceSet.InstanceSet.size(); i++)
		{
			UINT DirtyInstanceIndex = DirtyInstanceSet.InstanceSet[i];
			auto pInstance = m_pScene->GetInstance(DirtyInstanceIndex);
			_ASSERTE(pInstance);
			InstanceDataMap[DirtyInstanceIndex].World = pInstance->GetTransformMatrix();
		}
	}
}

void CSDFSceneRender::DrawQuad(CGraphicsCommandList* pCommandList)
{
	_ASSERTE(pCommandList);

	__UpdateBuffer();

	CCameraBasic* pCamera = m_pScene->GetCamera(m_RenderConfig.ActiveCameraIndex);
	_ASSERTE(pCamera);
	m_CBPerframe.View = pCamera->GetViewXMM();
	m_CBPerframe.Proj = pCamera->GetProjectXMM();
	m_CBPerframe.InvProj = XMMatrixInverse(&XMMatrixDeterminant(m_CBPerframe.Proj), m_CBPerframe.Proj);
	XMFLOAT3 CamPos = pCamera->GetPositionXMF3();
	m_CBPerframe.CameraPosW = { CamPos.x, CamPos.y, CamPos.z, 1.0f };

	pCommandList->TransitionResource(*m_pPositionMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*m_pNormMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*m_pModelIDAndInstanceIDMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);

	pCommandList->SetVertexBuffer(0, m_QuadVertexBufferView);
	pCommandList->SetIndexBuffer(m_QuadIndexBufferView);
	pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->SetConstantBuffer(kSceneCB, m_SceneConstantBuffer.GetGpuVirtualAddress());
	pCommandList->SetDynamicConstantBufferView(kPerFrameCB, sizeof(SPerframe), &m_CBPerframe);

	pCommandList->SetDynamicDescriptor(kPositionMapSRV, 0, m_pPositionMap->GetSRV());
	pCommandList->SetDynamicDescriptor(kNormMapSRV, 0, m_pNormMap->GetSRV());
	pCommandList->SetDynamicDescriptor(kModelIDAndInstanceIDMapSRV, 0, m_pModelIDAndInstanceIDMap->GetSRV());
	pCommandList->SetBufferSRV(kLightDataSRV, m_SDFLightDataBuffer);
	pCommandList->SetBufferSRV(kModelDataSRV, m_ModelDataBuffer);
	pCommandList->SetBufferSRV(kInstanceDataSRV, m_InstanceDataUploadBuffer.GetCurrentBuffer());
	pCommandList->SetDynamicDescriptors(kMeshSDFSRVs, 0, (UINT)m_SDFSrvSet.size(), m_SDFSrvSet.data());
	pCommandList->DrawIndexed(6, 0, 0);
}
