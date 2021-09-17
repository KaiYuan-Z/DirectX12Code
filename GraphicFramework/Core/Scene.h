#pragma once
#include "Utility.h"
#include "MathUtility.h"
#include "ObjectLibrary.h"
#include "ModelInstanceLibrary.h"
#include "CameraBasic.h"
#include "Light.h"
#include "BoundingFrustum.h"
#include "GrassManager.h"
#include "SceneCommon.h"

struct SModelRenderList
{
	UINT HostModelIndex;
	std::vector<UINT> InstanceIndexSet;
};

struct SInstanceSet
{
	long long FrameID = 0xffffffff;
	std::vector<UINT> InstanceSet;
};

struct SModelRenderListSet
{
	long long FrameID = 0xffffffff;
	UINT CameraIndex = 0xffffffff;
	std::vector<SModelRenderList> RenderListSet;

	SModelRenderListSet() {}
	SModelRenderListSet(UINT cameraIndex) : CameraIndex(cameraIndex) {}
};

struct SLightRenderSetSet
{
	long long FrameID = 0xffffffff;
	UINT CameraIndex = 0xffffffff;
	std::vector<UINT> LightIndexSet;

	SLightRenderSetSet() {}
	SLightRenderSetSet(UINT cameraIndex) : CameraIndex(cameraIndex) {}
};

class CScene
{
public:
	CScene();
	CScene(const std::wstring& sceneName);
	~CScene();

	bool LoadScene(const std::wstring& configFile);

	bool SaveScene(const std::wstring& configFile, UINT exportOptions = ExportOptions::ExportAll);

	const std::wstring& GetSceneName() const { return m_SceneName; }
	void SetSceneName(const std::wstring name) { m_SceneName = name; }

	UINT AddModelToScene(const std::wstring& modelName);
	
	UINT AddInstance(const std::wstring& instanceName, const std::wstring& hostModelName);
	bool DeleteInstance(const std::wstring& instanceName);
	UINT QueryInstanceIndexInScene(const std::wstring& instanceName) const;
	UINT GetInstanceCount() const { return m_InstanceLib.GetObjectCount(); }
	CModelInstance* GetInstance(UINT instanceIndexInScene) const;
	CModelInstance* GetInstance(const std::wstring& instanceName) const;
	bool IsInstanceExist(const std::wstring& instanceName) const;
	void OnInstanceUpdate(UINT frameID, UINT instanceIndexInScene);
	const SInstanceSet& GetRecentDirtyInstanceSet() const { return m_RecentDirtyInstanceSet; }

	UINT QueryModelIndexInScene(const std::wstring& modelName) const;
	UINT GetModelCountInScene() const { return m_ModelInstanceRecordLibs.GetObjectCount(); }
	CModel* GetModelInScene(UINT modelIndexInScene) const;
	CModel* GetModelInScene(const std::wstring& modelName) const;
	UINT GetModelInstanceCount(UINT modelIndexInScene) const;
	UINT GetModelInstanceCount(const std::wstring& hostModelName) const;
	CModelInstance* GetModelInstance(UINT modelIndexInScene, UINT instanceIndexInModel) const;
	
	UINT AddLight(const std::wstring& lightName, UINT lightType);
	bool DeleteLight(const std::wstring& lightName);
	UINT QueryLightIndex(const std::wstring& lightName) const;
	CLight* GetLight(UINT lightIndex) const;
	bool IsLightExist(const std::wstring& lightName) const;
	UINT GetLightCount() const;

	UINT AddCamera(const std::wstring& cameraName, UINT cameraType);
	bool DeleteCamera(const std::wstring& cameraName);
	UINT QueryCameraIndex(const std::wstring& cameraName) const;
	CCameraBasic* GetCamera(UINT cameraIndex) const;
	bool IsCameraExist(const std::wstring& cameraName) const;
	UINT GetCameraCount() const;

	void SetActiveCamera(UINT activeCameraIndex);
	const SModelRenderListSet& GetActiveCameraModelRenderListSet() const;
	const SLightRenderSetSet& GetActiveCameraLightRenderSet() const;

	CGrassManager& FetchGrassManager() { return m_GrassManager; }

	virtual void OnFrameUpdate();	
	virtual void OnFramePostProcess();

protected:
	std::wstring m_SceneName = L"";
	UINT m_ActiveCameraIndex = 0;

	CGrassManager m_GrassManager;

	TObjectLibrary<CModelInstance*> m_InstanceLib;
	TObjectLibrary<CLight*> m_LightLib;
	TObjectLibrary<CCameraBasic*> m_CameraLib;
	TObjectLibrary<CModelInstanceRecordLibrary*> m_ModelInstanceRecordLibs;

	SInstanceSet m_InstanceUpdateRecordSet;
	SInstanceSet m_InstanceDeferredUpdateRecordSet;
	SInstanceSet m_RecentDirtyInstanceSet;

	mutable std::vector<SModelRenderListSet> m_ModelRenderListSets;
	mutable std::vector<SLightRenderSetSet> m_LightRenderSets;
	
	CBoundingFrustum m_ActiveCameraFrustum;

	CModelInstanceRecordLibrary* __GetOrCreateModelInstanceLib(CModel* pHostModel);

	void __ReleaseResources();
};