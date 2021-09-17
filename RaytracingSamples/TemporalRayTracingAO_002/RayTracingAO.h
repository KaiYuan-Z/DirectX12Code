#pragma once
#include "../../GraphicFramework/RaytracingUtility/RaytracingScene.h"
#include "../../GraphicFramework/RaytracingUtility/RaytracingSceneRenderHelper.h"
#include "../../GraphicFramework/RaytracingUtility/RayTracingPipelineStateObject.h"
#include "../../GraphicFramework/RaytracingUtility/RaytracingDispatchRaysDescriptor.h"
#include "../../GraphicFramework/Core/DXSample.h"
#include "../../GraphicFramework/Core/MathUtility.h"
#include "../../GraphicFramework/Core/CameraWalkthrough.h"
#include "../../GraphicFramework/Core/Model.h"
#include "../../GraphicFramework/Core/ColorBuffer.h"
#include "../../GraphicFramework/Effects/GBufferEffect.h"
#include "../../GraphicFramework/Effects/ImageEffect.h"
#include "../../GraphicFramework/Effects/FxaaEffect.h"
#include "../../GraphicFramework/Effects/MarkDynamicEffect.h"
#include "../../GraphicFramework/Effects/CalcAoDiffEffect.h"
#include "../../GraphicFramework/Effects/AtrousWaveletTransfromBlurEffect.h"
#include "../../GraphicFramework/Effects/RayTracedAoTemporalSupersamplingEffect.h"
#include "../../GraphicFramework/Effects/BilateralBlurEffect.h"
#include "../../GraphicFramework/Core/SceneLightDeferredRender.h"
#include "../../GraphicFramework/Effects/ShadowMapEffect.h"
#include "../../GraphicFramework/Effects/SkyEffect.h"
#include "MotionManager.h"

using namespace DirectX;



class CRayTracingAO : public CDXSample
{
	struct SRayGenCB
	{
		float	AORadius = 0.2f;
		float	MinT = 0.005f;
		float   Pad1 = 0;
		float   Pad2 = 0;
		UINT	HighSampleNum = 16;
		UINT    MediumSampleNum = 8;
		UINT	LowSampleNum = 2;
		UINT	MaxAccumCount = 100;

		SRayGenCB() 
		{
		}

		SRayGenCB(float aoRadius,
			float minT,
			UINT highSampleNum,
			UINT mediumSampleNum,
			UINT lowSampleNum,
			UINT maxAccumCount
		)
		{
			AORadius = aoRadius;
			MinT = minT;
			HighSampleNum = highSampleNum;
			MediumSampleNum = mediumSampleNum;
			LowSampleNum = lowSampleNum;
			MaxAccumCount = maxAccumCount;
		}
	};

	struct SConfigSet
	{
		SRayGenCB AoRayConfig;
		bool OpenAoDiffTest = false;
	};

	struct SCameraParamter
	{
		XMFLOAT3 Pos;
		XMFLOAT3 Up;
		XMFLOAT3 Target;

		SCameraParamter()
		{
		}

		SCameraParamter(XMFLOAT3 pos, XMFLOAT3 up, XMFLOAT3 target)
		{
			Pos = pos;
			Up = up;
			Target = target;
		}
	};

	enum EDisplayMode
	{
		NewAoMap,
		AuccumedAoMap,
		FinalAoMap
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
	CGBufferEffect m_GBufferEffect;
	CRayTracedAoTemporalSupersamplingEffect m_AccumEffect;
	CImageEffect m_ImageEffect;
	CImageEffect m_ImageEffectDebugAccum;
	CImageEffect m_ImageEffectDebugAoDiff;
	CImageEffect m_ImageEffectDebugDynamicMark;
	CFxaaEffect m_FxaaEffect;
	CMarkDynamicEffect m_MarkDynamic;
	CBlurEffect m_BlurAoDiffEffect;
	CAtrousWaveletTransfromBlurEffect m_AtrousWaveletTransfromBlurEffect;
	CCalcAoDiffEffect m_CalcAoDiffEffect;
	CBilateralBlurEffect m_BilateralBlurEffect;
	CShadowMapEffect m_ShadowMapEffect;
	CSkyEffect m_SkyEffect;

	//
	// RayTracing Resources
	//
	RaytracingUtility::CRaytracingScene m_Scene;
	RaytracingUtility::CRaytracingSceneRenderHelper m_SceneRenderHelper;
	RaytracingUtility::CRayTracingPipelineStateObject m_RayTracingStateObject;
	RaytracingUtility::CRaytracingDispatchRaysDescriptor m_DispatchRaysDesc;
	CRootSignature m_RayTracingRootSignature;
	CRootSignature m_RaytracingLocalRootSignature;
	SRayGenCB m_CBRayGen;
	UINT m_UserRootParmStartIndex = 0;

	CColorBuffer m_SkyColorBuffer;
	CColorBuffer m_OutputBuffer;
	CColorBuffer m_PreSampleCacheBuffer;
	CColorBuffer m_HaltonIndexBuffer;
	CColorBuffer m_BlurOutputBuffer;
	CColorBuffer m_FinalOutputBuffer;
	CColorBuffer m_ColorBuffer;
	CDepthBuffer m_DepthBuffer;
	CDoubleColorBuffer m_DoubleAoAccumBuffer;

	EDisplayMode m_DisplayMode = FinalAoMap;

	CGraphicsPSO m_PSO;
	CRootSignature m_RootSignature;
	CRootSignature m_RtLocalRootSignature;
	CSceneLightDeferredRender m_SceneRender;
	UINT m_RootStartIndex = 0;
	UINT m_SampleStartIndex = 0;

	CModelInstance* m_pBusInst1 = nullptr;
	CMotionManager m_BusInst1Motion;
	CModelInstance* m_pBusInst2 = nullptr;
	CMotionManager m_BusInst2Motion;

	CModelInstance* m_pCar1Inst1 = nullptr;
	CMotionManager m_Car1Inst1Motion;
	CModelInstance* m_pCar1Inst2 = nullptr;
	CMotionManager m_Car1Inst2Motion;

	CModelInstance* m_pUaz1Inst1 = nullptr;
	CMotionManager m_Uaz1Inst1Motion;
	CModelInstance* m_pUaz1Inst2 = nullptr;
	CMotionManager m_Uaz1Inst2Motion;

	bool m_CarMove = true;
	bool m_DebugInfo = false;
	bool m_EnableAo = true;
	bool m_EnableShadow = true;
	bool m_ShowAoBounding = false;
	bool m_EabbleGrassAnim = true;

	SConfigSet m_Config1 = { {0.2f, 0.005f, 16, 8, 2, 150}, true};
	SConfigSet m_Config2 = { {0.5f, 0.005f, 16, 8, 2, 150}, true };
	SConfigSet m_Config3 = { {0.2f, 0.005f, 1, 1, 1, 150}, false };
	SConfigSet m_Config4 = { {0.5f, 0.005f, 1, 1, 1, 150}, false };

	SCameraParamter m_CamParam1 = { {-32.1578, 1.80477, 7.96202},{-0.496909, 0.858656, -0.125665},{-40.4823, -3.32076, 5.85695} };
	SCameraParamter m_CamParam2 = { {-21.8472, 5.07032, -42.3166},{0.178613, 0.896996, 0.404345},{-18.2226, 0.649944, -34.1115} };

	void __InitializeCommonResources();
	void __InitializeRasterizerResources();
	void __InitializeRayTracingResources();
	void __InitRenderBuffers();

	void __SetConfigSet(const SConfigSet& configSet) { m_CBRayGen = configSet.AoRayConfig; m_CalcAoDiffEffect.SetOpenState(configSet.OpenAoDiffTest); }

	void __SetCameraParamter(const SCameraParamter& paramter);

	//
	// GUI
	//
	void __UpdateGUI();
};