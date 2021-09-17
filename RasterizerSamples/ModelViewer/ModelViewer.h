#pragma once
#include "../../GraphicFramework/Core/DXSample.h"
#include "../../GraphicFramework/Core/MathUtility.h"
#include "../../GraphicFramework/Core/CameraWalkthrough.h"
#include "../../GraphicFramework/Core/Scene.h"
#include "../../GraphicFramework/Core/SceneForwardRender.h"
#include "../../GraphicFramework/Effects/ShadowMapEffect.h"
#include "../../GraphicFramework/Effects/SkyEffect.h"

using namespace DirectX;

class ModelViewer : public CDXSample
{
public:
	ModelViewer(std::wstring name);

	// Messages
	virtual void OnInit() override;
	virtual void OnKeyDown(UINT8 key) override;
	virtual void OnMouseMove(UINT x, UINT y) override;
	virtual void OnRightButtonDown(UINT x, UINT y) override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;
	virtual void OnDestroy() override {};

	static const UINT c_MaxModelCount = 100;
	static const UINT c_MaxInstanceCount = 100;
	static const UINT c_MaxGrassPatchCount = 100;
	static const UINT c_MaxLightCount = 30;

private:
	UINT m_RootParamStartIndex = 0;

	CCameraWalkthrough* m_pCamera = nullptr;

	CGraphicsPSO m_PSO;
	CRootSignature m_RootSignature;

	CScene m_Scene;
	CSceneForwardRender m_SceneRender;

	CShadowMapEffect m_ShadowMapEffect;
	CSkyEffect m_SkyEffect;

	bool m_EnableHighLight = true;

	UINT m_CurrentModelCount = 0;
	std::vector<std::string> m_ModelNameSet;
	const char* m_ModelNameList[c_MaxModelCount];

	UINT m_SelectedInstanceIndex = 0;
	UINT m_CurrentInstanceCount = 0;
	std::vector<std::string> m_InstanceNameSet;
	const char* m_InstanceNameList[c_MaxInstanceCount];

	UINT m_SelectedGrassPatchIndex = 0;
	UINT m_CurrentGrassPatchCount = 0;
	std::vector<std::string> m_GrassPatchNameSet;
	const char* m_GrassPatchNameList[c_MaxGrassPatchCount];

	void __BuildRootSignature();
	void __BuildPSO();
	void __BuildScene();
	void __InitDisplayList();
	void __UpdateGUI();
};
