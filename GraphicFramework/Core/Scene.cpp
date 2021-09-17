#include "Scene.h"
#include "ModelManager.h"
#include "CameraWalkthrough.h"
#include "GraphicsCore.h"
#include "SceneImporter.h"
#include "SceneExporter.h"

CScene::CScene() : CScene(L"")
{
}

CScene::CScene(const std::wstring& sceneName) : m_SceneName(sceneName)
{
}

CScene::~CScene()
{
	__ReleaseResources();
}

bool CScene::LoadScene(const std::wstring& configFile)
{
	m_GrassManager.Initialize();
	CSceneImporter sceneImporter(*this);
	return sceneImporter.LoadConfig(MakeStr(configFile));
}

bool CScene::SaveScene(const std::wstring& configFile, UINT exportOptions)
{
	CSceneExporter sceneExporter(*this);
	return sceneExporter.SaveConfig(MakeStr(configFile), exportOptions);
}

UINT CScene::AddModelToScene(const std::wstring& modelName)
{
	UINT modelIndex = ModelManager::QueryModelIndex(modelName);
	if (modelIndex == INVALID_OBJECT_INDEX)
	{
		THROW_MSG_IF_FALSE(modelIndex != INVALID_OBJECT_INDEX, L"The model has not been loaded yet.\n");
		return modelIndex;
	}
	CModel* pModel = ModelManager::GetBasicModel(modelIndex);
	_ASSERTE(pModel);

	__GetOrCreateModelInstanceLib(pModel);
	
	return m_ModelInstanceRecordLibs.QueryObjectEntry(modelName);
}

UINT CScene::AddInstance(const std::wstring& instanceName, const std::wstring& hostModelName)
{
	UINT modelIndex = ModelManager::QueryModelIndex(hostModelName);
	if (modelIndex == INVALID_OBJECT_INDEX)
	{
		THROW_MSG_IF_FALSE(modelIndex != INVALID_OBJECT_INDEX, L"The model has not been loaded yet.\n");
		return modelIndex;
	}
	CModel* pModel = ModelManager::GetBasicModel(modelIndex);
	_ASSERTE(pModel);
	
	UINT instanceIndex = m_InstanceLib.QueryObjectEntry(instanceName);
	if (instanceIndex != INVALID_OBJECT_INDEX)
	{
		THROW_MSG_IF_FALSE(false, L"The instance name already exists.\n");
		return INVALID_OBJECT_INDEX;
	}
	CModelInstance* pInstance = new CModelInstance(instanceName, pModel, this);
	_ASSERTE(pInstance);
	instanceIndex = m_InstanceLib.AddObject(instanceName, pInstance);
	pInstance->SetIndex(instanceIndex);

	CModelInstanceRecordLibrary* pInstanceRecordLib = __GetOrCreateModelInstanceLib(pModel);
	_ASSERTE(pInstanceRecordLib);
	pInstanceRecordLib->AddInstanceRecord(instanceName, instanceIndex);
	
	return instanceIndex;
}

bool CScene::DeleteInstance(const std::wstring& instanceName)
{
	UINT instanceIndex = m_InstanceLib.QueryObjectEntry(instanceName);
	if (instanceIndex == INVALID_OBJECT_INDEX) return false;
	CModelInstance* pDeleteInstance = m_InstanceLib[instanceIndex];
	_ASSERTE(pDeleteInstance);

	CModel* pModel = pDeleteInstance->GetModel();
	_ASSERTE(pModel);
	UINT instanceRecordLibIndex = m_ModelInstanceRecordLibs.QueryObjectEntry(pModel->GetModelName());
	CModelInstanceRecordLibrary* pInstanceRecordLib = m_ModelInstanceRecordLibs[instanceRecordLibIndex];
	_ASSERTE(pInstanceRecordLib);

	if (!m_InstanceLib.DeleteObject(instanceIndex)) return false;

	if (!pInstanceRecordLib->DeleteInstanceRecord(instanceName)) return false;

	if (instanceIndex < m_InstanceLib.GetObjectCount())
	{	
		CModelInstance* pSwapInstance = m_InstanceLib[instanceIndex];
		_ASSERTE(pSwapInstance);
		pSwapInstance->SetIndex(instanceIndex);
		CModel* pSwapInstanceHostModel = pSwapInstance->GetModel();
		_ASSERTE(pSwapInstanceHostModel);
		UINT instanceRecordLibIndex2 = m_ModelInstanceRecordLibs.QueryObjectEntry(pSwapInstanceHostModel->GetModelName());
		CModelInstanceRecordLibrary* pInstanceRecordLib2 = m_ModelInstanceRecordLibs[instanceRecordLibIndex2];
		_ASSERTE(pInstanceRecordLib2);
		if (!pInstanceRecordLib2->SetInstanceRecord(m_InstanceLib[instanceIndex]->GetName(), instanceIndex)) return false;
	}

	if (pDeleteInstance) delete pDeleteInstance;

	return true;
}

UINT CScene::QueryInstanceIndexInScene(const std::wstring& instanceName) const
{	
	return m_InstanceLib.QueryObjectEntry(instanceName);
}

CModelInstance* CScene::GetInstance(UINT instanceIndexInScene) const
{
	return m_InstanceLib[instanceIndexInScene];
}

CModelInstance* CScene::GetInstance(const std::wstring& instanceName) const
{
	UINT instanceIndex = m_InstanceLib.QueryObjectEntry(instanceName);
	if (instanceIndex == INVALID_OBJECT_INDEX) return nullptr;
	return m_InstanceLib[instanceIndex];
}

bool CScene::IsInstanceExist(const std::wstring& instanceName) const
{
	return m_InstanceLib.IsObjectExist(instanceName);
}

void CScene::OnInstanceUpdate(UINT frameID, UINT instanceIndexInScene)
{
	m_InstanceUpdateRecordSet.FrameID = frameID;
	m_InstanceUpdateRecordSet.InstanceSet.push_back(instanceIndexInScene);
}

UINT CScene::QueryModelIndexInScene(const std::wstring& modelName) const
{
	return m_ModelInstanceRecordLibs.QueryObjectEntry(modelName);
}

CModel* CScene::GetModelInScene(UINT modelIndexInScene) const
{
	CModelInstanceRecordLibrary* pInstanceRecordLib = m_ModelInstanceRecordLibs[modelIndexInScene];
	_ASSERTE(pInstanceRecordLib);
	return pInstanceRecordLib->GetHostModel();
}

CModel* CScene::GetModelInScene(const std::wstring& modelName) const
{
	UINT instanceRecordLibIndex = m_ModelInstanceRecordLibs.QueryObjectEntry(modelName);
	if (instanceRecordLibIndex == INVALID_OBJECT_INDEX) return nullptr;
	CModelInstanceRecordLibrary* pInstanceRecordLib = m_ModelInstanceRecordLibs[instanceRecordLibIndex];
	_ASSERTE(pInstanceRecordLib);
	return pInstanceRecordLib->GetHostModel();
}

UINT CScene::GetModelInstanceCount(UINT modelIndexInScene) const
{
	CModelInstanceRecordLibrary* pInstanceRecordLib = m_ModelInstanceRecordLibs[modelIndexInScene];
	_ASSERTE(pInstanceRecordLib);
	return pInstanceRecordLib->GetInstanceRecordCount();
}

UINT CScene::GetModelInstanceCount(const std::wstring& hostModelName) const
{
	UINT instanceRecordLibIndex = m_ModelInstanceRecordLibs.QueryObjectEntry(hostModelName);
	if (instanceRecordLibIndex == INVALID_OBJECT_INDEX) return 0;
	CModelInstanceRecordLibrary* pInstanceRecordLib = m_ModelInstanceRecordLibs[instanceRecordLibIndex];
	_ASSERTE(pInstanceRecordLib);
	return pInstanceRecordLib->GetInstanceRecordCount();
}

CModelInstance* CScene::GetModelInstance(UINT modelIndexInScene, UINT instanceIndexInModel) const
{
	CModelInstanceRecordLibrary* pInstanceRecordLib = m_ModelInstanceRecordLibs[modelIndexInScene];
	_ASSERTE(pInstanceRecordLib);
	UINT instanceIndex = pInstanceRecordLib->GetInstanceRecord(instanceIndexInModel);
	return m_InstanceLib[instanceIndex];
}

UINT CScene::AddLight(const std::wstring& lightName, UINT lightType)
{
	if (!m_LightLib.IsObjectExist(lightName))
	{
		CLight* pLight = CLight::CreateLight(lightName, lightType);
		_ASSERTE(pLight);
		return m_LightLib.AddObject(lightName, pLight);
	}
	else
	{
		THROW_MSG_IF_FALSE(false, L"The Light name already exists.\n");
		return INVALID_OBJECT_INDEX;
	}
}

bool CScene::DeleteLight(const std::wstring& lightName)
{
	return m_LightLib.DeleteObject(lightName);
}

UINT CScene::QueryLightIndex(const std::wstring& lightName) const
{
	return m_LightLib.QueryObjectEntry(lightName);
}

CLight* CScene::GetLight(UINT lightIndex) const
{
	return m_LightLib[lightIndex];
}

bool CScene::IsLightExist(const std::wstring& lightName) const
{
	return m_LightLib.IsObjectExist(lightName);
}

UINT CScene::GetLightCount() const
{
	return m_LightLib.GetObjectCount();
}

UINT CScene::AddCamera(const std::wstring& cameraName, UINT cameraType)
{
	if (!m_CameraLib.IsObjectExist(cameraName))
	{
		CCameraBasic* pCamera = CCameraBasic::CreateCamera(cameraName, cameraType);
		_ASSERTE(pCamera);
		UINT cameraIndex =  m_CameraLib.AddObject(cameraName, pCamera);
		_ASSERTE(cameraIndex != INVALID_OBJECT_INDEX);
		m_ModelRenderListSets.push_back(SModelRenderListSet(cameraIndex));
		m_LightRenderSets.push_back(SLightRenderSetSet(cameraIndex));
		return cameraIndex;
	}
	else
	{
		THROW_MSG_IF_FALSE(false, L"The camera name already exists.\n");
		return INVALID_OBJECT_INDEX;
	}
}

bool CScene::DeleteCamera(const std::wstring& cameraName) 
{
	UINT deleteIndex = m_CameraLib.QueryObjectEntry(cameraName);
	if (deleteIndex == INVALID_OBJECT_INDEX) return false;
	UINT lastIndex = m_CameraLib.GetObjectCount() - 1;

	if (!m_CameraLib.DeleteObject(cameraName)) return false;

	std::swap(m_ModelRenderListSets[deleteIndex], m_ModelRenderListSets[lastIndex]);
	std::swap(m_LightRenderSets[deleteIndex], m_LightRenderSets[lastIndex]);
	UINT reSize = lastIndex;
	m_ModelRenderListSets.resize(reSize);
	m_LightRenderSets.resize(reSize);

	return true;
}

UINT CScene::QueryCameraIndex(const std::wstring& cameraName) const
{
	return m_CameraLib.QueryObjectEntry(cameraName);
}

CCameraBasic* CScene::GetCamera(UINT cameraIndex) const
{
	return m_CameraLib[cameraIndex];
}

bool CScene::IsCameraExist(const std::wstring& cameraName) const
{
	return m_CameraLib.IsObjectExist(cameraName);
}

UINT CScene::GetCameraCount() const
{
	return m_CameraLib.GetObjectCount();
}

void CScene::SetActiveCamera(UINT activeCameraIndex)
{
	CCameraBasic* pCamera = GetCamera(activeCameraIndex);
	_ASSERTE(pCamera);
	m_ActiveCameraIndex = activeCameraIndex;
	const DirectX::XMMATRIX InvView = XMMatrixInverse(nullptr, pCamera->GetViewXMM());
	m_ActiveCameraFrustum.SetFrustum(pCamera->GetBoundingFrustum(), InvView);
}

const SModelRenderListSet& CScene::GetActiveCameraModelRenderListSet() const
{
	_ASSERTE(m_ActiveCameraIndex < m_ModelRenderListSets.size());
	_ASSERTE(m_ModelRenderListSets[m_ActiveCameraIndex].CameraIndex == m_ActiveCameraIndex);

	if (m_ModelRenderListSets[m_ActiveCameraIndex].FrameID != GraphicsCore::GetFrameID())
	{
		m_ModelRenderListSets[m_ActiveCameraIndex].FrameID = GraphicsCore::GetFrameID();

		auto& renderListSet = m_ModelRenderListSets[m_ActiveCameraIndex].RenderListSet;
		UINT modelCount = GetModelCountInScene();
		if (renderListSet.size() != modelCount) renderListSet.resize(modelCount);

		UINT modelInstanceCount = 0;
		UINT modelInstanceIndex = 0;
		CModelInstance* pModelInstance0 = nullptr;
		CModelInstance* pModelInstance1 = nullptr;
		bool res0 = false, res1 = false;
		for (UINT modelIndex = 0; modelIndex < modelCount; modelIndex++)
		{
			renderListSet[modelIndex].HostModelIndex = modelIndex;
			renderListSet[modelIndex].InstanceIndexSet.clear();
			modelInstanceCount = GetModelInstanceCount(modelIndex);

			for (modelInstanceIndex = 0; modelInstanceIndex < modelInstanceCount; modelInstanceIndex+=2)
			{
				pModelInstance0 = GetModelInstance(modelIndex, modelInstanceIndex);
				_ASSERTE(pModelInstance0);
				const BoundingSphere& sphere0 = pModelInstance0->GetBoundingObject().boundingSphere;

				if (modelInstanceIndex + 1 < modelInstanceCount)
				{
					pModelInstance1 = GetModelInstance(modelIndex, modelInstanceIndex + 1);
					_ASSERTE(pModelInstance1);
					const BoundingSphere& sphere1 = pModelInstance1->GetBoundingObject().boundingSphere;

					m_ActiveCameraFrustum.Contains(sphere0.Center, sphere0.Radius, sphere1.Center, sphere1.Radius, res0, res1);
					res0 = true; res1 = true;
					if (res0) { renderListSet[modelIndex].InstanceIndexSet.push_back(pModelInstance0->GetIndex()); res0 = false; }
					if (res1) { renderListSet[modelIndex].InstanceIndexSet.push_back(pModelInstance1->GetIndex()); res1 = false; }
				}
				else
				{
					res0 = true; res1 = true;
					m_ActiveCameraFrustum.Contains(sphere0.Center, sphere0.Radius, XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f, res0, res1);
					if (res0) { renderListSet[modelIndex].InstanceIndexSet.push_back(pModelInstance0->GetIndex()); res0 = false; }
				}
			}
		}
	}

	return m_ModelRenderListSets[m_ActiveCameraIndex];
}

const SLightRenderSetSet& CScene::GetActiveCameraLightRenderSet() const
{
	_ASSERTE(m_ActiveCameraIndex < m_LightRenderSets.size());
	_ASSERTE(m_LightRenderSets[m_ActiveCameraIndex].CameraIndex == m_ActiveCameraIndex);

	if (m_LightRenderSets[m_ActiveCameraIndex].FrameID != GraphicsCore::GetFrameID())
	{	
		m_LightRenderSets[m_ActiveCameraIndex].FrameID = GraphicsCore::GetFrameID();
		
		auto& lightIndexSet = m_LightRenderSets[m_ActiveCameraIndex].LightIndexSet;
		lightIndexSet.clear();
		UINT lightCount = m_LightLib.GetObjectCount();
		for (size_t lightIndex = 0; lightIndex < lightCount; lightIndex++)
		{
			//Fix: Light culling.
			lightIndexSet.push_back((UINT)lightIndex);
		}
	}

	return m_LightRenderSets[m_ActiveCameraIndex];
}

void CScene::OnFrameUpdate()
{
	UINT frameID = GraphicsCore::GetFrameID();

	if (m_GrassManager.GetGrarssPatchCount() > 0)
	{
		m_GrassManager.UpdateGrassPatch();
	}
	
	// Update current dirty instance.
	if (m_InstanceUpdateRecordSet.InstanceSet.size() > 0)
	{
		m_RecentDirtyInstanceSet.FrameID = m_InstanceUpdateRecordSet.FrameID;
		m_RecentDirtyInstanceSet.InstanceSet.clear();

		for (size_t i = 0; i < m_InstanceUpdateRecordSet.InstanceSet.size(); i++)
		{
			UINT instanceIndex = m_InstanceUpdateRecordSet.InstanceSet[i];
			CModelInstance* pInstance = m_InstanceLib[instanceIndex];
			_ASSERTE(pInstance);
			if (pInstance->GetAndResetCurDirtyUnProcessHint())
			{
				// Fix: Update Hierarchical Acceleration Structure

				m_RecentDirtyInstanceSet.InstanceSet.push_back(instanceIndex);
			}
		}

		m_InstanceUpdateRecordSet.FrameID = 0xffffffff;
		m_InstanceUpdateRecordSet.InstanceSet.clear();
	}
	else
	{
		if (m_RecentDirtyInstanceSet.FrameID != 0xffffffff)
		{
			m_RecentDirtyInstanceSet.FrameID = 0xffffffff;
			m_RecentDirtyInstanceSet.InstanceSet.clear();
		}
	}

	// Update camera.
	for (UINT i = 0; i < GetCameraCount(); i++)
	{
		CCameraBasic* pCamera = GetCamera(i); _ASSERTE(pCamera);
		pCamera->OnFrameUpdate();
	}
}

void CScene::OnFramePostProcess()
{
	for (UINT i = 0; i < GetCameraCount(); i++)
	{
		CCameraBasic* pCamera = GetCamera(i); _ASSERTE(pCamera);
		pCamera->OnFramePostProcess();
	}
}

CModelInstanceRecordLibrary* CScene::__GetOrCreateModelInstanceLib(CModel* pHostModel)
{
	_ASSERTE(pHostModel);
	UINT instanceLibIndex = m_ModelInstanceRecordLibs.QueryObjectEntry(pHostModel->GetModelName());
	if (instanceLibIndex == INVALID_OBJECT_INDEX)
	{
		CModelInstanceRecordLibrary* pNewInstanceLib = new CModelInstanceRecordLibrary(pHostModel);
		_ASSERTE(pNewInstanceLib);
		instanceLibIndex = m_ModelInstanceRecordLibs.AddObject(pHostModel->GetModelName(), pNewInstanceLib);
	}
	return m_ModelInstanceRecordLibs[instanceLibIndex];
}

void CScene::__ReleaseResources()
{
	for (UINT instanceIndex = 0; instanceIndex < m_InstanceLib.GetObjectCount(); instanceIndex++)
	{
		if (m_InstanceLib[instanceIndex]) delete m_InstanceLib[instanceIndex];
	}

	for (UINT lightIndex = 0; lightIndex < m_LightLib.GetObjectCount(); lightIndex++)
	{
		if (m_LightLib[lightIndex]) delete m_LightLib[lightIndex];
	}

	for (UINT cameraIndex = 0; cameraIndex < m_CameraLib.GetObjectCount(); cameraIndex++)
	{
		if (m_CameraLib[cameraIndex]) delete m_CameraLib[cameraIndex];
	}

	for (UINT instanceLibIndex = 0; instanceLibIndex < m_ModelInstanceRecordLibs.GetObjectCount(); instanceLibIndex++)
	{
		if (m_ModelInstanceRecordLibs[instanceLibIndex]) delete m_ModelInstanceRecordLibs[instanceLibIndex];
	}
}
