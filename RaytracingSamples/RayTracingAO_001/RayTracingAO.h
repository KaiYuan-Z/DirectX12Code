#pragma once
#include "../../GraphicFramework/RaytracingUtility/RaytracingScene.h"
#include "../../GraphicFramework/RaytracingUtility/RaytracingSceneRenderHelper.h"
#include "../../GraphicFramework/RaytracingUtility/RayTracingPipelineStateObject.h"
#include "../../GraphicFramework/RaytracingUtility/RaytracingShaderTable.h"
#include "../../GraphicFramework/Core/DXSample.h"
#include "../../GraphicFramework/Core/MathUtility.h"
#include "../../GraphicFramework/Core/CameraWalkthrough.h"
#include "../../GraphicFramework/Core/Model.h"
#include "../../GraphicFramework/Core/ColorBuffer.h"
#include "../../GraphicFramework/Effects/GeometryOnlyGBufferEffect.h"
#include "../../GraphicFramework/Effects/ImageEffect.h"

using namespace DirectX;


class CRayTracingAO : public CDXSample
{
	struct SRayGenCB
	{
		float AORadius;
		float MinT;
		int  AONumRays;
		bool OpenCosineSample = false;
	};

public:
	CRayTracingAO(std::wstring name);
	~CRayTracingAO();

	// Messages
	virtual void OnInit() override;
	virtual void OnKeyDown(UINT8 key) override;
	virtual void OnMouseMove(UINT x, UINT y) override;
	virtual void OnRightButtonDown(UINT x, UINT y) override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;
	virtual void OnPostProcess() override;
	virtual void OnDestroy() override {};

private:

	//
	// Common Resources
	//
	CCameraWalkthrough* m_pCamera = nullptr;

	//
	// Rasterizer Resources
	//
	CGeometryOnlyGBufferEffect m_GBufferEffect;
	CImageEffect m_ImageEffect;

	//
	// RayTracing Resources
	//
	RaytracingUtility::CRaytracingScene m_Scene;
	RaytracingUtility::CRaytracingSceneRenderHelper m_SceneRenderHelper;
	RaytracingUtility::CRayTracingPipelineStateObject m_RayTracingStateObject;
	RaytracingUtility::CRaytracingShaderTable m_HitShaderTable;
	RaytracingUtility::CRaytracingShaderTable m_MissShaderTable;
	RaytracingUtility::CRaytracingShaderTable m_RayGenShaderTable;
	D3D12_DISPATCH_RAYS_DESC m_DispatchRaysDesc;
	CRootSignature m_RtGlobalRootSignature;
	CRootSignature m_RtLocalRootSignature;
	SRayGenCB m_CBRayGen;
	CColorBuffer m_OutputBuffer;
	UINT m_UserRootParmStartIndex = 0;

	void __InitializeCommonResources();
	void __InitializeRasterizerResources();
	void __InitializeRayTracingResources();

	//
	// GUI
	//

	void __UpdateGUI();
};