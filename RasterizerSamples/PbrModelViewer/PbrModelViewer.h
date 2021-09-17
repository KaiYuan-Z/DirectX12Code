#pragma once
#include "../../GraphicFramework/Core/DXSample.h"
#include "../../GraphicFramework/Core/MathUtility.h"
#include "../../GraphicFramework/Core/CameraWalkthrough.h"
#include "../../GraphicFramework/Core/Scene.h"
#include "../../GraphicFramework/Core/SceneForwardRender.h"

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

private:
	CCameraWalkthrough* m_pCamera = nullptr;

	CGraphicsPSO m_PSO;
	CRootSignature m_RootSignature;

	CScene m_Scene;
	CSceneForwardRender m_SceneRender;

	void __BuildRootSignature();
	void __BuildPSO();
	void __BuildScene();

};
