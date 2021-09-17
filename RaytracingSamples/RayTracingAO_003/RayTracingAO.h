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
#include "../../GraphicFramework/Core/StructuredBuffer.h"
#include "../../GraphicFramework/Effects/ImageEffect.h"

using namespace DirectX;



class CRayTracingAO : public CDXSample
{
	struct SRayGenCB
	{
		float AORadius = 12.0f;
		float MinT = 0.001f;
		int   PixelNumRays = 4;
		int   AONumRays = 4;
		int	  HaltonBase1 = 5;
		int   HaltonBase2 = 7;
		int   HaltonIndexStep = 64;
		int   HaltonStartIndex = 0;
	};

	struct SSceneCB
	{
		XMMATRIX ViewProj;
		XMMATRIX InvViewProj;
		XMFLOAT4 CameraPosition;
		UINT  FrameCount;
	};

	struct SModelMeshInfo
	{
		UINT IndexOffsetBytes;
		UINT VertexOffsetBytes;
		UINT VertexStride;
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
	CImageEffect m_ImageEffect;


	//
	// RayTracing Resources
	//
	RaytracingUtility::CRaytracingScene m_Scene;
	RaytracingUtility::CRaytracingSceneRenderHelper m_SceneRenderHelper;
	RaytracingUtility::CRayTracingPipelineStateObject m_RayTracingStateObject;
	RaytracingUtility::CRaytracingShaderTable m_MissShaderTable;
	RaytracingUtility::CRaytracingShaderTable m_HitGroupShaderTable;
	RaytracingUtility::CRaytracingShaderTable m_RayGenShaderTable;
	D3D12_DISPATCH_RAYS_DESC m_DispatchRaysDesc;

	CRootSignature m_RayTracingRootSignature;
	CRootSignature m_RaytracingLocalRootSignature;

	CColorBuffer m_OutputBuffer;

	SRayGenCB m_CBRayGen;

	UINT m_UserRootParmStartIndex = 0;

	void __InitializeCommonResources();
	void __InitializeRasterizerResources();
	void __InitializeRayTracingResources();

	//
	// GUI
	//
	void __UpdateGUI();
};