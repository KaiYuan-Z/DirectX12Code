#pragma once
#include "../../GraphicFramework/Core/DXSample.h"
#include "../../GraphicFramework/Core/Scene.h"
#include "../../GraphicFramework/Core/CameraWalkthrough.h"
#include "../../GraphicFramework/Effects/GeometryOnlyGBufferEffect.h"
#include "../../GraphicFramework/Effects/HBAOPlusEffect.h"
#include "../../GraphicFramework/Effects/ImageEffect.h"

using namespace DirectX;

class CSSAOTest : public CDXSample
{
public:
	CSSAOTest(std::wstring name);

	// Messages
	virtual void OnInit() override;
	virtual void OnKeyDown(UINT8 key) override;
	virtual void OnMouseMove(UINT x, UINT y) override;
	virtual void OnRightButtonDown(UINT x, UINT y) override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;
	virtual void OnPostProcess() override;
	virtual void OnDestroy() override;

private:
	CCameraWalkthrough* m_pCamera = nullptr;

	CScene m_Scene;

	//Effect Resource
	CGeometryOnlyGBufferEffect m_GBufferEffect;
	CHBAOPlus m_SsaoEffect;
	CImageEffect m_ImageEffect;

	float m_AoTime = 0.0f;

	void __InitScene();
	void __InitEffectResource();
	void __UpdateGUI();
};
