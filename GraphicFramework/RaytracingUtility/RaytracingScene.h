#pragma once
#include "../Core/Scene.h"
#include "../Core/RenderCommon.h"
#include"../Core/ObjectLibrary.h"
#include "../Core/StructuredBuffer.h"
#include "../Core/ByteAddressBuffer.h"
#include "BottomLevelAccelerationStructure.h"
#include "TopLevelAccelerationStructure.h"
#include "RaytracingShaderTable.h"
#include "RaytracingPipelineStateObject.h"


namespace RaytracingUtility
{
	class CRaytracingScene : public CScene
	{
		struct SRtMaterial
		{
			XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
			XMFLOAT3 FresnelR0 = { 0.1f, 0.1f, 0.1f };
			float Shininess = 0.1f;

			UINT TexDiffuseIndex = 0;
			UINT TexSpecularIndex = 0;
			UINT TexEmissiveIndex = 0;
			UINT TexNormalIndex = 0;
			UINT TexLightmapIndex = 0;
			UINT TexReflectionIndex = 0;
			UINT Pad[2];
		};
		
		struct SRtMeshData
		{
			UINT MaterialIndex = 0;
			UINT IndexOffsetBytes = 0;
			UINT VertexOffsetBytes = 0;
			UINT VertexStride = 0;
			UINT IndexTypeSize = 0;
			UINT Pad[3];
		};

		struct SRtInstanceData
		{
			UINT InstanceIndex = 0;
			UINT ModelIndex = 0;
			UINT InstanceTag = 0;
			UINT FirstMeshGlobalIndex = 0;
		};

		struct SRtSceneInfo
		{
			UINT TotalInstanceCount = 0;
			UINT TotalLightCount = 0;
			UINT TotalTLACount = 0;// Top Level Acceleration Structure Count.
		};

	public:
		CRaytracingScene();
		CRaytracingScene(const std::wstring& name);
		~CRaytracingScene();

		void InitRaytracingScene();

		UINT AddTopLevelAccelerationStructure(const std::wstring& TLAName, bool enableUpdate, UINT hitProgCount);
	
		virtual void OnFrameUpdate() override;

		UINT QueryTLAIndexByName(const std::wstring& TLAName);

	private:
		std::vector<CBottomLevelAccelerationStructure> m_RtBLAs;
		TObjectLibrary<CTopLevelAccelerationStructure> m_RtTLAs;
		std::vector<UINT> m_TLAHitProgCounts;
		std::vector<UINT> m_TLAHitProgRecordCounts;

		std::vector<UINT> m_RtModelFirstMeshGlobalIndexSet;
		std::vector<UINT> m_RtInstanceUpdateFrameIdSet;
		std::vector<XMMATRIX> m_RtInstanceWorldSet;
		std::vector<SRtInstanceData> m_RtInstanceDataSet;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_RtTexDescSet;

		CStructuredBuffer m_RtMaterialBuffer;
		CStructuredBuffer m_RtMeshDataBuffer;
		CStructuredBuffer m_RtInstanceDataBuffer;
		CStructuredBuffer m_RtLightDataBuffer;
		CStructuredBuffer m_RtVertexBuffer;
		CByteAddressBuffer m_RtIndexBuffer;

		SRtSceneInfo m_RtSceneInfo;

		bool m_IsInitialized = false;

		UINT m_GrassVertexDataOffset = 0;
		UINT m_GrassVertexDataSize = 0;

		void __BuildBottomLevelAccelerationStructure();
		void __BuildVertexAndIndexData();
		void __BuildInstanceData();
		void __BuildLightData();
		void __BuildMeshData();

		void __BuildHitProgLocalRootSignature(CRootSignature& rootSignature);
		void __BuildMeshHitShaderTable(UINT TLAIndex, CRayTracingPipelineStateObject* pRtPSO, const std::vector<std::wstring>& hitGroupNameSet, CRaytracingShaderTable& outShaderTable);

		friend class CRaytracingSceneRenderHelper;
		friend class CRaytracingShaderTable;
	};
}

