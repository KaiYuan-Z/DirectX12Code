#pragma once
#include "../../GraphicFramework/RaytracingUtility/RaytracingShaderTable.h"
#include "../../GraphicFramework/RaytracingUtility/TopLevelAccelerationStructure.h"
#include "../../GraphicFramework/RaytracingUtility/BottomLevelAccelerationStructure.h"
#include "../../GraphicFramework/RaytracingUtility/RayTracingPipelineStateObject.h"
#include "../../GraphicFramework/Core/DXSample.h"
#include "../../GraphicFramework/Core/MathUtility.h"
#include "../../GraphicFramework/Core/CameraWalkthrough.h"
#include "../../GraphicFramework/Core/Model.h"
#include "../../GraphicFramework/Core/DoubleColorBuffer.h"
#include "../../GraphicFramework/Core/ColorBuffer.h"
#include "../../GraphicFramework/Core/StructuredBuffer.h"
#include "../../GraphicFramework/Effects/GBufferEffect.h"
#include "../../GraphicFramework/Effects/ImageEffect.h"
#include "../../GraphicFramework/Effects/SimpleAccumulationEffect.h"

using namespace DirectX;


class CRayTracingAO : public CDXSample
{
	struct SRayGenCB
	{
		float AORadius;
		float MinT;
		int   PixelNumRays;
		int   AONumRays;
	};

	struct SSceneCB
	{
		XMMATRIX ViewProj;
		XMMATRIX InvViewProj;
		XMFLOAT4 CameraPosition;
		UINT	 FrameCount;
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
	virtual void OnDestroy() override {};

private:

	//
	// Common Resources
	//

	CCameraWalkthrough m_Camera;
	CModel m_Model;


	//
	// Rasterizer Resources
	//

	CSimpleAccumulationEffect m_SimpleAccumulationEffect;
	CImageEffect m_ImageEffect;

	CDoubleColorBuffer m_DoubleAccumBuffer;


	//
	// RayTracing Resources
	//

	// Raytracing state object
	RaytracingUtility::CRayTracingPipelineStateObject m_RayTracingStateObject;

	// Raytracing root signature
	CRootSignature m_RayTracingRootSignature;
	CRootSignature m_RaytracingLocalRootSignature;

	// Acceleration structure
	RaytracingUtility::CBottomLevelAccelerationStructure m_BottomLevelAccelerationStructure;
	RaytracingUtility::CTopLevelAccelerationStructure m_TopLevelAccelerationStructure;

	// RayTracing shader buffer
	CStructuredBuffer m_ModelMeshInfoBuffer;
	SRayGenCB m_CBRayGen;
	SSceneCB m_CBScene;

	// Raytracing output Buffer
	CColorBuffer m_AoOutputBuffer;

	// Shader tables
	RaytracingUtility::CRaytracingShaderTable m_MissShaderTable;
	RaytracingUtility::CRaytracingShaderTable m_HitGroupShaderTable;
	RaytracingUtility::CRaytracingShaderTable m_RayGenShaderTable;

	// Dispatch rays desc
	D3D12_DISPATCH_RAYS_DESC m_DispatchRaysDesc;

	// Max Accum Count
	const int m_MaxAccumCount = 64;

	void __InitializeCommonResources();
	void __InitializeRasterizerResources();
	void __InitializeRayTracingResources();


	//
	// GUI
	//

	void __UpdateGUI();
};