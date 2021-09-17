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
#include "Sampler.h"

using namespace DirectX;


class CRayTracingAO : public CDXSample
{
	struct SRayGenCB
	{
		float		AORadius = 12.0f;
		float		MinT = 0.01f;
		int			AONumRays = 16;
		bool		OpenMultiJitter = true;

		UINT		NumSampleSets = 1;
		UINT		NumSamplesPerSet = 1;
		UINT		NumPixelsPerDimPerSet = 1;
		UINT		Pad1;
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

	Samplers::MultiJittered m_randomSampler;
	UINT c_NumSampleSets = 1024;
	CStructuredBuffer m_SampleSets;

	void __InitializeCommonResources();
	void __InitializeRasterizerResources();
	void __InitializeRayTracingResources();
	void __InitializeRandomSamples();

	//
	// GUI
	//

	void __UpdateGUI();
};