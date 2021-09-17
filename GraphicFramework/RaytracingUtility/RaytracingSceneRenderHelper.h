#pragma once
#include "RaytracingScene.h"
#include "../Core/GraphicsCore.h"
#include "../Core/SwappedUserDescriptorHeap.h"
#include "../Core/ConstantBuffer.h"

namespace RaytracingUtility
{
	class CRaytracingSceneRenderHelper
	{
		enum ERtSceneRenderGlobalRootParameter
		{
			kAccelerationStructureSrv = 0,

			kRtSceneRenderSrvs,

			kDynamicInstanceUpdateFrameIdSrv,

			kInstanceWorldSrv,

			kTextureDescTable,

			kPerSceneCB,		
			kPerFrameCB,
			kHaltonParamCB,

			nRtSceneRenderRootParameterNum
		};

		enum ERtSceneRenderSRVRegister
		{
			kAccelerationStructureSrvRegister = 0,
			kMaterialBufferSrvRegister,
			kMeshDataBufferSrvRegister,
			kInstanceDataBufferSrvRegister,
			kLightDataBufferSrvRegister,
			kVertexBufferSrvRegister,
			kIndexBufferSrvRegister,
			kHaltonPerm3SrvRegister,
			kDynamicInstanceUpdateFrameIdSrvRegister,
			kInstanceWorldSrvRegister,
			kTextureDescTableRegister
		};

		enum ERtSceneRenderCBRegister
		{
			kPreSceneCbRegister = 0,
			kPerFrameCbRegister,
			kHaltonParamCbRegister,
		};

		enum ERtSceneRenderStaticSampler
		{
			kLinearSampler = 0,

			kStaticSamplerNum
		};

		struct SPerSceneCB
		{
			UINT InstanceCount = 0;
			UINT LightCount = 0;
			UINT HitProgCount = 0;
			UINT Pad;
		};

		struct SPerFrameCB
		{
			XMMATRIX ViewProj = XMMatrixIdentity();
			XMMATRIX InvViewProj = XMMatrixIdentity();
			XMFLOAT3 CameraPosition = { 0.0f, 0.0f, 0.0f };
			UINT FrameID = 0;
			XMFLOAT2 Jitter = { 0.0f, 0.0f };
			XMFLOAT2 Pad;
		};

	public:
		CRaytracingSceneRenderHelper();
		~CRaytracingSceneRenderHelper();

		void InitRender(CRaytracingScene* pRtScene, UINT activeTLAIndex, UINT activeCameraIndex = 0);
		void ResetHaltonParams(UINT DispatchSizeX, UINT DispatchSizeY);

		UINT GetStaticSamplerCount() const { return (UINT)kStaticSamplerNum; }
		UINT GetStaticSamplerStartRegister() const { return (UINT)kLinearSampler; }

		UINT GetRootParameterCount() const { return (UINT)nRtSceneRenderRootParameterNum; }
		UINT GetRootParameterStartIndex() const { return (UINT)(kAccelerationStructureSrv); }

		CRaytracingScene* GetHostRaytracingScene() const { return m_pRtScene; }

		void ConfigurateGlobalRootSignature(CRootSignature& rootSignature);
		void SetGlobalRootParameter(CComputeCommandList* pCommandList, bool setGeometryDataOnly = false);

		void BuildHitProgLocalRootSignature(CRootSignature& rootSignature);
		void BuildMeshHitShaderTable(UINT TLAIndex, CRayTracingPipelineStateObject* pRtPSO, const std::vector<std::wstring>& hitGroupNameSet, CRaytracingShaderTable& outShaderTable);

	private:
		const UINT m_RtTexDescFirstHandle = 0;
		UINT m_RtTexDescCount = 0;
		UINT m_ActiveTLAIndex = 0xffffffff;

		CRaytracingScene* m_pRtScene = nullptr;
		CSwappedUserDescriptorHeap m_SwappedHeap;
		
		SPerSceneCB m_PerSceneCB;
		SPerFrameCB m_PerFrameCB;

		CConstantBuffer m_PerSceneConstantBuffer;
		CConstantBuffer m_HaltonParamConstantBuffer;

		CStructuredBuffer m_HaltonPerm3StructuredBuffer;
	};
}

